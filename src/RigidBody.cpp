/*
 * Implementation file of the GJK/EPA algorithm
 */
#include "pch.h"

#include "Log.h"
#include "RigidBody.h"

// Settings for the GJK and EPA algorithm
constexpr int MAX_GJK_ITERATIONS = 64;

constexpr float EPA_TOLERANCE = 0.01f; // glm::epsilon<float>();
constexpr int EPA_MAX_NUM_FACES = 128;
constexpr int EPA_MAX_NUM_LOOSE_EDGES = 32;
constexpr int EPA_MAX_NUM_ITERATIONS = 128;
constexpr float EPA_BIAS = 0.00001f;

// GJK algorithm, heavily 'inspired' by https://github.com/kevinmoran/GJK and
// the articles on winter.dev (https://winter.dev/articles/gjk-algorithm,
// https://winter.dev/articles/epa-algorithm).
// Returns true if two colliders are intersecting. Has optional EPAnormal output
// param; If supplied EPA will be used to find the minimum translation vector to
// separate coll1 from coll2

constexpr bool GJK_impl(const Collider *p, const Collider *q,
                        const glm::vec3 &initial_axis, glm::vec3 *EPAnormal);

bool Intersect(const Collider *p, const Collider *q,
               const glm::vec3 &initial_axis) {
  return GJK_impl(p, q, initial_axis, nullptr);
}
HitInfo Collide(const Collider *p, const Collider *q,
                const glm::vec3 &initial_axis) {
  glm::vec3 normal;
  bool res = GJK_impl(p, q, initial_axis, &normal);
  return {glm::normalize(normal), glm::length(normal), res};
}

#define OWN 2
#define GJK_C 1
#define WINTER_DEV 0
#define EPA_METHOD GJK_C

// Kevin's implementation of the Gilbert-Johnson-Keerthi intersection algorithm
// and the Expanding Polytope Algorithm
// Most useful references (Huge thanks to all the authors):

// "Implementing GJK" by Casey Muratori:
// The best description of the algorithm from the ground up
// https://www.youtube.com/watch?v=Qupqu1xe7Io

// "Implementing a GJK Intersection Query" by Phill Djonov
// Interesting tips for implementing the algorithm
// http://vec3.ca/gjk/implementation/

// "GJK Algorithm 3D" by Sergiu Craitoiu
// Has nice diagrams to visualise the tetrahedral case
// http://in2gpu.com/2014/05/18/gjk-algorithm-3d/

// "GJK + Expanding Polytope Algorithm - Implementation and Visualization"
// Good breakdown of EPA with demo for visualisation
// https://www.youtube.com/watch?v=6rgiPrzqt9w
//-----------------------------------------------------------------------------

// Expanding Polytope Algorithm. Used to find the mtv of two intersecting
// colliders using the final simplex obtained with the GJK algorithm
glm::vec3 EPA(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
              const Collider *coll1, const Collider *coll2);

constexpr bool Triangle(std::array<glm::vec3, 4> &simplex, glm::vec3 &direction,
                        int &simplex_dimension);
constexpr bool Tetrahedron(std::array<glm::vec3, 4> &simplex,
                           glm::vec3 &direction);

constexpr bool SameDirection(const glm::vec3 &a, const glm::vec3 &b) {
  return glm::dot(a, b) > 0.0f;
}

// Find the vector perpendicular to a in the direction of b
constexpr glm::vec3 DoubleCross(const glm::vec3 &a, const glm::vec3 &b) {
  return glm::cross(glm::cross(a, b), a);
}

// helper functions
constexpr bool Triangle(glm::vec3 &A, glm::vec3 &B, glm::vec3 &C, glm::vec3 &D,
                        glm::vec3 &direction, int &simplex_dimension) {
  // Makes sure A is always the one that can be overwritten next
  /* Required winding order is counterclockwise:
   *  b
   *  |\
   *  | \
   *  |  a
   *  | /
   *  |/
   *  c
   */
  glm::vec3 AO = -A;
  glm::vec3 AB = B - A;
  glm::vec3 AC = C - A;
  glm::vec3 ABC = glm::cross(AB, AC); // Triangles normal

  simplex_dimension = 2;

  // It's guaranteed that A is on the right side of the origin, but the other
  // points can be at the wrong side too

  // We change these point early so the chances of finding the right tetrahedron
  // are higher later, it's more efficient in the long run

  // Check if C is at the wrong side of the origin
  if (SameDirection(glm::cross(AB, ABC), AO)) {
    // Find a better alternative to C
    C = A;
    direction = DoubleCross(AB, AO);
    return false;
  }
  // Check if B is at the wrong side of the origin
  if (SameDirection(glm::cross(ABC, AC), AO)) {
    // Find a better alternative to B
    B = A;
    direction = DoubleCross(AC, AO);
    return false;
  }

  // I would like a tetrahedron please...
  simplex_dimension = 3;

  if (SameDirection(ABC, AO)) {
    // New point will be above the triangle, leave simplex as-is, but make sure
    // A is free
    D = C;
    C = B;
    B = A;
    direction = ABC;
  } else {
    // New point will be below the triangle, want that to be above, so rotate
    // the points around, and make sure A is free for next point
    D = B;
    B = A;
    direction = -ABC;
  }
  return false;
}
// A is the peak of the tetrahedron, BCD is the base (CCW ordering)
// Origin is above BCD and below A
constexpr bool Tetrahedron(glm::vec3 &A, glm::vec3 &B, glm::vec3 &C,
                           glm::vec3 &D, glm::vec3 &direction) {
  glm::vec3 AO = -A;
  glm::vec3 AB = B - A;
  glm::vec3 AC = C - A;
  glm::vec3 AD = D - A;

  // Normals of the three new faces, all pointing outside the tetrahedron
  glm::vec3 ABC = glm::cross(AB, AC);
  glm::vec3 ACD = glm::cross(AC, AD);
  glm::vec3 ADB = glm::cross(AD, AB);

  // In front of ABC, remove D
  if (SameDirection(ABC, AO)) {
    D = C;
    C = B;
    B = A;
    direction = ABC;
    return false;
  }
  // In front of ACD, remove B
  if (SameDirection(ACD, AO)) {
    B = A;
    direction = ACD;
    return false;
  }
  // In front of ADB, remove C
  if (SameDirection(ADB, AO)) {
    C = D;
    D = B;
    B = A;
    direction = ADB;
    return false;
  }
  // Cannot be in front of BCD, because A must be after the origin
  return true;
}

struct Face {
  glm::vec3 vertices[3];
  glm::vec3 normal;
};
#if EPA_METHOD == OWN
// Assumes the two are colliding
glm::vec3 EPA(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 D,
              const Collider *p, const Collider *q) {
  glm::vec3 AB = B - A;
  glm::vec3 AC = C - A;
  glm::vec3 AD = D - A;
  glm::vec3 BD = D - B;
  glm::vec3 BC = C - B;

  Face faces[EPA_MAX_NUM_FACES];
  faces[0] = Face{{A, B, C}, glm::normalize(glm::cross(AB, AC))};
  faces[1] = Face{{A, C, D}, glm::normalize(glm::cross(AC, AD))};
  faces[2] = Face{{A, D, B}, glm::normalize(glm::cross(AD, AB))};
  faces[3] = Face{{B, D, C}, glm::normalize(glm::cross(BD, BC))};
  size_t numFaces = 4;

  Face &closestFace = faces[0];

  for (int iteration = 0; iteration < EPA_MAX_NUM_ITERATIONS; iteration++) {
    // Find closest face to the origin
    float minDist = glm::dot(faces[0].vertices[0], faces[0].normal);
    closestFace = faces[0];
    for (size_t i = 1; i < numFaces; i++) {
      Face &face = faces[i];
      float dist = dot(face.vertices[0], face.normal);
      if (dist < minDist) {
        minDist = dist;
        closestFace = face;
      }
    }

    // Get the support point facing the normal of that face
    glm::vec3 support = Support(p, q, closestFace.normal);
    // convergence (new point is not further from the origin)
    if (glm::dot(support, closestFace.normal) - minDist < EPA_TOLERANCE) {
      return closestFace.normal * glm::dot(support, closestFace.normal);
    }

    // keep track of edges we need to fix after removing faces
    size_t numLooseEdges = 0;
    std::pair<glm::vec3, glm::vec3> looseEdges[EPA_MAX_NUM_LOOSE_EDGES];

    // Old-school for loop because we're modifying inside the loop
    for (size_t i = 0; i < numFaces; i++) {
      if (SameDirection(faces[i].normal, support - faces[i].vertices[0])) {
        // Triangle faces support direction, need to remove it
        for (int j = 0; j < 3; j++) {
          std::pair<glm::vec3, glm::vec3> currentEdge{
              faces[i].vertices[j], faces[i].vertices[(j + 1) % 3]};
          bool foundEdge = false;

          for (size_t k = 0; k < numLooseEdges; k++) {
            if (currentEdge.first == looseEdges[k].second &&
                currentEdge.second == looseEdges[k].first) {
              // Edge is already in the list, need to remove it
              // This assumes an edge can only be shared by 2 triangles (which
              // should be true).
              // Also assumes shared edge will be reversed in the triangles
              // (which is true if every triangle is wound CCW)
              // Like so: / \ B /
              //         / A \ /

              // Modifying inside the for loop, but we're leaving anyway
              looseEdges[k] = looseEdges[numLooseEdges - 1];
              k = numLooseEdges;
              numLooseEdges--;
              foundEdge = true;
            }
          }
          // Add current edge to the list
          if (!foundEdge) {
            if (numLooseEdges >= EPA_MAX_NUM_LOOSE_EDGES) {
              NG_ERROR("Too many loose edges");
              break;
            }
            looseEdges[numLooseEdges] = currentEdge;
            numLooseEdges++;
          }
        }
        // Remove triangle i from the list
        faces[i] = faces[numLooseEdges - 1];
        numLooseEdges--;
        i--;
      }
    }
    // Reconstruct polytope
    for (size_t e = 0; e < numLooseEdges; e++) {
      if (numFaces >= EPA_MAX_NUM_FACES) {
        NG_ERROR("Too many faces");
        break;
      }
      auto &edge = looseEdges[e];
      faces[numFaces] = {{edge.first, edge.second, support},
                         glm::normalize(glm::cross(edge.first - edge.second,
                                                   edge.first - support))};

      // Check the normal to maintain CCW ordering
      if (glm::dot(faces[numFaces].vertices[0], faces[numFaces].normal) +
              EPA_BIAS <
          0.0f) {
        std::swap(faces[numFaces].vertices[0], faces[numFaces].vertices[1]);
        faces[numFaces].normal = -faces[numFaces].normal;
      }
      numFaces++;
    }
  }

  NG_WARN("EPA did not converge");
  // return closest point
  return closestFace.normal *
         glm::dot(closestFace.vertices[0], closestFace.normal);
}
#elif EPA_METHOD == GJK_C
glm::vec3 EPA(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
              const Collider *coll1, const Collider *coll2) {
  Face faces[EPA_MAX_NUM_FACES]; // Array of faces, each with 3 verts and a
                                 // normal

  // Init with final simplex from GJK
  faces[0].vertices[0] = a;
  faces[0].vertices[1] = b;
  faces[0].vertices[2] = c;
  faces[0].normal = glm::normalize(glm::cross(b - a, c - a)); // ABC
  faces[1].vertices[0] = a;
  faces[1].vertices[1] = c;
  faces[1].vertices[2] = d;
  faces[1].normal = glm::normalize(glm::cross(c - a, d - a)); // ACD
  faces[2].vertices[0] = a;
  faces[2].vertices[1] = d;
  faces[2].vertices[2] = b;
  faces[2].normal = glm::normalize(glm::cross(d - a, b - a)); // ADB
  faces[3].vertices[0] = b;
  faces[3].vertices[1] = d;
  faces[3].vertices[2] = c;
  faces[3].normal = glm::normalize(glm::cross(d - b, c - b)); // BDC

  int num_faces = 4;
  int closest_face;

  for (int iterations = 0; iterations < EPA_MAX_NUM_ITERATIONS; iterations++) {
    // Find face that's closest to origin
    float min_dist = dot(faces[0].vertices[0], faces[0].normal);
    closest_face = 0;
    for (int i = 1; i < num_faces; i++) {
      float dist = dot(faces[i].vertices[0], faces[i].normal);
      if (dist < min_dist) {
        min_dist = dist;
        closest_face = i;
      }
    }

    // search normal to face that's closest to origin
    glm::vec3 search_dir = faces[closest_face].normal;
    glm::vec3 p = coll2->Support(search_dir) - coll1->Support(-search_dir);

    if (dot(p, search_dir) - min_dist < EPA_TOLERANCE) {
      // Convergence (new point is not significantly further from origin)
      return faces[closest_face].normal *
             dot(p, search_dir); // dot vertex with normal to resolve collision
                                 // along normal!
    }

    std::pair<glm::vec3, glm::vec3>
        loose_edges[EPA_MAX_NUM_LOOSE_EDGES]; // keep track of edges we need
                                              // to fix after removing faces
    int num_loose_edges = 0;

    // Find all triangles that are facing p
    for (int i = 0; i < num_faces; i++) {
      if (dot(faces[i].normal, p - faces[i].vertices[0]) >
          0) // triangle i faces p, remove it
      {
        // Add removed triangle's edges to loose edge list.
        // If it's already there, remove it (both triangles it belonged to are
        // gone)
        for (int j = 0; j < 3; j++) // Three edges per face
        {
          std::pair<glm::vec3, glm::vec3> current_edge = {
              faces[i].vertices[j], faces[i].vertices[(j + 1) % 3]};
          bool found_edge = false;
          for (int k = 0; k < num_loose_edges;
               k++) // Check if current edge is already in list
          {
            if (loose_edges[k].second == current_edge.first &&
                loose_edges[k].first == current_edge.second) {
              // Edge is already in the list, remove it
              // THIS ASSUMES EDGE CAN ONLY BE SHARED BY 2 TRIANGLES (which
              // should be true) THIS ALSO ASSUMES SHARED EDGE WILL BE REVERSED
              // IN THE TRIANGLES (which should be true provided every triangle
              // is wound CCW)
              loose_edges[k].first = loose_edges[num_loose_edges - 1]
                                         .first; // Overwrite current edge
              loose_edges[k].second = loose_edges[num_loose_edges - 1]
                                          .second; // with last edge in list
              num_loose_edges--;
              found_edge = true;
              k = num_loose_edges; // exit loop because edge can only be shared
                                   // once
            }
          } // endfor loose_edges

          if (!found_edge) { // add current edge to list
            // assert(num_loose_edges<EPA_MAX_NUM_LOOSE_EDGES);
            if (num_loose_edges >= EPA_MAX_NUM_LOOSE_EDGES)
              break;
            loose_edges[num_loose_edges].first = current_edge.first;
            loose_edges[num_loose_edges].second = current_edge.second;
            num_loose_edges++;
          }
        }

        // Remove triangle i from list
        faces[i].vertices[0] = faces[num_faces - 1].vertices[0];
        faces[i].vertices[1] = faces[num_faces - 1].vertices[1];
        faces[i].vertices[2] = faces[num_faces - 1].vertices[2];
        faces[i].normal = faces[num_faces - 1].normal;
        num_faces--;
        i--;
      } // endif p can see triangle i
    }   // endfor num_faces

    // Reconstruct polytope with p added
    for (int i = 0; i < num_loose_edges; i++) {
      // assert(num_faces<EPA_MAX_NUM_FACES);
      if (num_faces >= EPA_MAX_NUM_FACES)
        break;
      faces[num_faces].vertices[0] = loose_edges[i].first;
      faces[num_faces].vertices[1] = loose_edges[i].second;
      faces[num_faces].vertices[2] = p;
      faces[num_faces].normal = glm::normalize(
          glm::cross(loose_edges[i].first - loose_edges[i].second,
                     loose_edges[i].first - p));

      // Check for wrong normal to maintain CCW winding
      if (dot(faces[num_faces].vertices[0], faces[num_faces].normal) +
              EPA_BIAS <
          0) {
        glm::vec3 temp = faces[num_faces].vertices[0];
        faces[num_faces].vertices[0] = faces[num_faces].vertices[1];
        faces[num_faces].vertices[1] = temp;
        faces[num_faces].normal = -faces[num_faces].normal;
      }
      num_faces++;
    }
  } // End for iterations
  NG_WARN("EPA did not converge");
  // Return most recent closest point
  return faces[closest_face].normal *
         dot(faces[closest_face].vertices[0], faces[closest_face].normal);
}
#elif EPA_METHOD == WINTER_DEV
void AddIfUniqueEdge(std::vector<std::pair<size_t, size_t>> &edges,
                     const std::vector<size_t> &faces, size_t a, size_t b) {
  auto reverse = std::find(              //      0--<--3
      edges.begin(),                     //     / \ B /   A: 2-0
      edges.end(),                       //    / A \ /    B: 0-2
      std::make_pair(faces[b], faces[a]) //   1-->--2
  );

  if (reverse != edges.end()) {
    edges.erase(reverse);
  } else {
    edges.emplace_back(faces[a], faces[b]);
  }
}
std::pair<std::vector<glm::vec4>, size_t>
GetFaceNormals(const std::vector<glm::vec3> &polytope,
               const std::vector<size_t> &faces) {
  std::vector<glm::vec4> normals;
  size_t minTriangle = 0;
  float minDistance = FLT_MAX;

  for (size_t i = 0; i < faces.size(); i += 3) {
    glm::vec3 a = polytope[faces[i]];
    glm::vec3 b = polytope[faces[i + 1]];
    glm::vec3 c = polytope[faces[i + 2]];

    glm::vec3 normal = normalize(cross(b - a, c - a));
    float distance = dot(normal, a);

    if (distance < 0) {
      normal *= -1;
      distance *= -1;
    }

    normals.emplace_back(normal, distance);

    if (distance < minDistance) {
      minTriangle = i / 3;
      minDistance = distance;
    }
  }

  return {normals, minTriangle};
}

glm::vec3 EPA(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 D,
              const Collider *colliderA, const Collider *colliderB) {
  std::vector<glm::vec3> polytope = {A, B, C, D};
  std::vector<size_t> faces = {0, 1, 2, 0, 3, 1, 0, 2, 3, 1, 3, 2};

  // list: vec4(normal, distance), index: min distance
  auto [normals, minFace] = GetFaceNormals(polytope, faces);
  glm::vec3 minNormal;
  float minDistance = FLT_MAX;

  while (minDistance == FLT_MAX) {
    minNormal = glm::vec3(normals[minFace]);
    minDistance = normals[minFace].w;

    glm::vec3 support = Support(colliderA, colliderB, minNormal);
    float sDistance = dot(minNormal, support);

    if (abs(sDistance - minDistance) > 0.001f) {
      minDistance = FLT_MAX;
      std::vector<std::pair<size_t, size_t>> uniqueEdges;

      for (size_t i = 0; i < normals.size(); i++) {
        if (glm::dot(glm::vec3(normals[i]), support) >
            glm::dot(glm::vec3(normals[i]), polytope[faces[i * 3]])) {
          size_t f = i * 3;

          AddIfUniqueEdge(uniqueEdges, faces, f, f + 1);
          AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
          AddIfUniqueEdge(uniqueEdges, faces, f + 2, f);

          faces[f + 2] = faces.back();
          faces.pop_back();
          faces[f + 1] = faces.back();
          faces.pop_back();
          faces[f] = faces.back();
          faces.pop_back();

          normals[i] = normals.back(); // pop-erase
          normals.pop_back();

          i--;
        }
      }
      std::vector<size_t> newFaces;
      for (auto [edgeIndex1, edgeIndex2] : uniqueEdges) {
        newFaces.push_back(edgeIndex1);
        newFaces.push_back(edgeIndex2);
        newFaces.push_back(polytope.size());
      }

      polytope.push_back(support);

      auto [newNormals, newMinFace] = GetFaceNormals(polytope, newFaces);
      float oldMinDistance = FLT_MAX;
      for (size_t i = 0; i < normals.size(); i++) {
        if (normals[i].w < oldMinDistance) {
          oldMinDistance = normals[i].w;
          minFace = i;
        }
      }

      if (newNormals[newMinFace].w < oldMinDistance) {
        minFace = newMinFace + normals.size();
      }

      faces.insert(faces.end(), newFaces.begin(), newFaces.end());
      normals.insert(normals.end(), newNormals.begin(), newNormals.end());
    }
  }

  return glm::normalize(minNormal) * (minDistance + 0.001f);
}
#endif
constexpr bool GJK_impl(const Collider *p, const Collider *q,
                        const glm::vec3 &initial_axis, glm::vec3 *EPAnormal) {

  glm::vec3 searchDirection = initial_axis;
  glm::vec3 A, B, C, D;
  C = Support(p, q, searchDirection);
  searchDirection = -C; // search in direction of origin
  // Get second point for a line segment simplex
  B = Support(p, q, searchDirection);
  glm::vec3 OB = -B;
  if (SameDirection(OB, searchDirection)) {
    // we didn't reach the origin, won't enclose it
    return false;
  }
  glm::vec3 BC = C - B;
  // search perpendicular to line segment towards origin
  searchDirection = DoubleCross(BC, OB);
  if (searchDirection == glm::vec3(0, 0, 0)) { // origin is on this line segment
    // Apparently any normal search vector will do?
    searchDirection = cross(BC, glm::vec3(1, 0, 0)); // normal with x-axis
    if (searchDirection == glm::vec3(0, 0, 0))
      searchDirection = cross(BC, glm::vec3(0, 0, -1)); // normal with z-axis
  }
  int simplexDimension = 2; // simplex dimension
  for (int iterations = 0; iterations < MAX_GJK_ITERATIONS; iterations++) {
    A = Support(p, q, searchDirection);
    if (SameDirection(searchDirection, -A)) {
      return false;
    } // we didn't reach the origin, won't enclose it

    simplexDimension = glm::min(simplexDimension + 1, 4);
    if (simplexDimension == 3) {
      Triangle(A, B, C, D, searchDirection, simplexDimension);
    } else if (Tetrahedron(A, B, C, D, searchDirection)) {
      if (EPAnormal != nullptr) {
        *EPAnormal = EPA(A, B, C, D, p, q);
      }
      return true;
    }
  } // endfor
  return false;
}
