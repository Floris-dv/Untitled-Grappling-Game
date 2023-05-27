#include "pch.h"
#define OVERLOAD_GLM_OSTREAM 0
#include "Log.h"
#include "Level.h"
#include "VertexData.h"
#include "imgui/imgui.h"
#include <imguizmo/ImGuizmo.h>
#include "SaveFile.h"
#include "Window.h"
#include "GrapplingCamera.h"

template<typename OStream>
inline OStream& operator<<(OStream& output, Level::Block const& input) {
	output << input.Start;
	output << input.End;
	output << input.Rotation;

	return output;
}

template<typename IStream>
inline IStream& operator>>(IStream& input, Level::Block& output) {
	input >> output.Start;
	input >> output.End;
	input >> output.Rotation;

	return input;
}

Level::Level(const glm::vec3& startPlatformSize, const Block& finishBox, std::vector<Block>&& blocks, std::unique_ptr<Material>&& instancedMaterial, std::unique_ptr<Material>&& normalMaterial, std::unique_ptr<Material>&& finishMaterial) :
	m_Blocks(std::move(blocks)), m_InstancedMaterial(std::move(instancedMaterial)), m_Material(std::move(normalMaterial)), m_FinishMaterial(std::move(finishMaterial)), m_AllowEditing(true)
{
	m_Blocks.insert(m_Blocks.begin(), { finishBox, { { -startPlatformSize.x * 0.5f, -startPlatformSize.y, -startPlatformSize.z * 0.5f}, {startPlatformSize.x * 0.5f, 0.0f, startPlatformSize.z * 0.5f} } });
	SetupMatrices();
	SetupÍnstanceVBO();
}

Level::Level(const std::string& levelFile, std::shared_ptr<Shader> instancedShader, std::shared_ptr<Shader> normalShader) : m_FileName(levelFile)
{
	std::ifstream file(levelFile, std::ios::binary);

	if (!file)
		NG_ERROR("File {} doesn't exist", levelFile);

	uint32_t version_nr;
	file >> version_nr;

	file >> m_AllowEditing;

	file >> m_Blocks;

	std::stringstream materialData = Material::GetDataFromFile(file);
	std::stringstream copy;
	copy << materialData.str();

	m_InstancedMaterial = std::make_unique<Material>(Material::Deserialize(copy, instancedShader));
	m_Material = std::make_unique<Material>(Material::Deserialize(materialData, normalShader));
	m_FinishMaterial = std::make_unique<Material>(Material::Deserialize(file, normalShader));

	SetupMatrices();
	SetupÍnstanceVBO();
}

void Level::Write(std::string_view levelFile)
{
	std::ofstream file(levelFile.data(), std::ios::binary);

	file << VERSION_NR << '\n';

	file << m_AllowEditing << '\n';

	file << m_Blocks;

	m_Material->Serialize(file);
	m_FinishMaterial->Serialize(file);
}

void Level::Render(Material* material, Material* finishMaterial) {
	Block::Object.DrawInstanced(material, true, m_Matrices.size() - 1); // finish box needs to be another color
	Block::Object.Draw(finishMaterial, m_Matrices.back(), true);
}

void Level::RenderOneByOne()
{
	for (const auto& mat : std::ranges::views::take(m_Matrices, m_Matrices.size() - 1))
		Block::Object.Draw(m_Material.get(), mat, true);

	Block::Object.Draw(m_FinishMaterial.get(), m_Matrices.back(), true);
}

void Level::RenderEditingMode(size_t& index)
{
	if (!m_AllowEditing)
		return;

	static constexpr ImGuiDataType size_t_DataType = std::is_same_v<size_t, uint32_t> ? ImGuiDataType_U32 : ImGuiDataType_U64;

	static ImGuizmo::OPERATION s_CurrentGizmoOperation(ImGuizmo::TRANSLATE);
	static ImGuizmo::MODE s_CurrentGizmoMode(ImGuizmo::WORLD);

	{
		const size_t numMatrices = m_Matrices.size();

		// TODO: Center this
		if (ImGui::Button("<"))
			index--;
		ImGui::SameLine();
		ImGui::DragScalar("##EMPTY", size_t_DataType, &index, 1.0f, 0, &numMatrices);
		ImGui::SameLine();
		if (ImGui::Button(">"))
			index++;
		index = (index + numMatrices) % numMatrices;
		ImGui::SameLine();
		if (ImGui::Button("+")) {
			m_Matrices.push_back(glm::identity<glm::mat4>());
			m_Blocks.push_back(Level::Block{});
			index = m_Matrices.size() - 1;
		}
	}
	float* matrix = glm::value_ptr(m_Matrices[index]);

	if (ImGui::IsKeyPressed(ImGuiKey_T))
		s_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
	if (ImGui::IsKeyPressed(ImGuiKey_R))
		s_CurrentGizmoOperation = ImGuizmo::ROTATE;
	if (ImGui::IsKeyPressed(ImGuiKey_E))
		s_CurrentGizmoOperation = ImGuizmo::SCALE;
	if (ImGui::RadioButton("Translate", s_CurrentGizmoOperation == ImGuizmo::TRANSLATE))
		s_CurrentGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", s_CurrentGizmoOperation == ImGuizmo::ROTATE))
		s_CurrentGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", s_CurrentGizmoOperation == ImGuizmo::SCALE))
		s_CurrentGizmoOperation = ImGuizmo::SCALE;
	glm::vec3 matrixTranslation, matrixRotation, matrixScale;
	ImGuizmo::DecomposeMatrixToComponents(matrix, glm::value_ptr(matrixTranslation), glm::value_ptr(matrixRotation), glm::value_ptr(matrixScale));
	ImGui::DragFloat3("Tr", glm::value_ptr(matrixTranslation), 1.0f);
	ImGui::DragFloat3("Rt", glm::value_ptr(matrixRotation), 0.1f);
	ImGui::DragFloat3("Sc", glm::value_ptr(matrixScale), 1.0f);
	ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(matrixTranslation), glm::value_ptr(matrixRotation), glm::value_ptr(matrixScale), matrix); // changes m_Matrices[index]

	ImGui::Text("Quaternion: %f %f %f %f", m_Blocks[index].Rotation.x, m_Blocks[index].Rotation.y, m_Blocks[index].Rotation.z, m_Blocks[index].Rotation.w);

	m_Blocks[index] = { matrixTranslation - matrixScale * 0.5f, matrixTranslation + matrixScale * 0.5f, glm::quat(glm::radians(matrixRotation)) };

	if (s_CurrentGizmoOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", s_CurrentGizmoMode == ImGuizmo::LOCAL))
			s_CurrentGizmoMode = ImGuizmo::LOCAL;
		ImGui::SameLine();
		if (ImGui::RadioButton("World", s_CurrentGizmoMode == ImGuizmo::WORLD))
			s_CurrentGizmoMode = ImGuizmo::WORLD;
	}
	static bool useSnap(false);
	if (ImGui::IsKeyPressed(ImGuiKey_Y))
		useSnap = !useSnap;
	ImGui::Checkbox("Enable snapping", &useSnap);
	ImGui::SameLine();
	static glm::vec3 snap{ 0.0f };
	switch (s_CurrentGizmoOperation)
	{
	case ImGuizmo::TRANSLATE:
		ImGui::DragFloat3("Snap", &snap.x);
		break;
	case ImGuizmo::ROTATE:
		ImGui::DragFloat("Angle Snap", &snap.x);
		break;
	case ImGuizmo::SCALE:
		ImGui::DragFloat("Scale Snap", &snap.x);
		break;
	}
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(glm::value_ptr(Camera::Get()->GetViewMatrix()), glm::value_ptr(Camera::Get()->GetProjMatrix()), s_CurrentGizmoOperation, s_CurrentGizmoMode, matrix, nullptr, useSnap ? &snap.x : nullptr);

	if (ImGui::Button("Save as")) {
		m_FileName = GetSaveFileNameAllPlatforms(Window::Get().GetGLFWWindow());
		Write(m_FileName);
	}
	if (!m_FileName.empty())
		if (ImGui::Button("Save"))
			Write(m_FileName);
}

struct OBB {
	glm::vec3 pos = glm::vec3{ 0.0f };
	glm::vec3 axisX{ 1.0f, 0.0f, 0.0f }, axisY{ 0.0f, 1.0f, 0.0f }, axisZ{ 0.0f, 0.0f, 1.0f };
	glm::vec3 halfSize = glm::vec3{ 0.5f };

	OBB() {}

	OBB(const Level::Block& block) {
		pos = (block.Start + block.End) * 0.5f;
		axisX = block.Rotation * glm::vec3(1.0f, 0.0f, 0.0f);
		axisY = block.Rotation * glm::vec3(0.0f, 1.0f, 0.0f);
		axisZ = block.Rotation * glm::vec3(0.0f, 0.0f, 1.0f);
		halfSize = (block.End - block.Start) * 0.5f;
	}
};

static bool CollisionCheck(const OBB& box1, const OBB& box2);

// Assumes camera is colliding with block
static void BounceOn(const Level::Block& block, GrapplingCamera& camera);

bool Level::UpdatePhysics(GrapplingCamera& camera)
{
	OBB cameraBox{};
	cameraBox.pos = camera.PhysicsPosition;
	// camera box has no rotation
	// TODO: add better constant factor for the camera block here
	cameraBox.halfSize = glm::vec3{ 0.2f };

	for (const Level::Block& block : std::views::take(m_Blocks, m_Blocks.size() - 1)) {
		if (CollisionCheck(cameraBox, OBB(block))) {
			ImGui::Text("Inside!!!");
			BounceOn(block, camera);
		}
	}

	if (CollisionCheck(cameraBox, OBB(m_Blocks.back())))
		return true;

	return false;
}

void Level::SetupÍnstanceVBO()
{
	std::shared_ptr<Material> s(nullptr);
	// TODO: don't include the last matrix (that uses another shader
	m_InstanceVBO = VertexBuffer((m_Matrices.size() - 1) * sizeof(m_Matrices[0]), &m_Matrices[1]);

	if (!Block::Object.IsValid())
		Block::Object = Object<SimpleVertex>(nullptr, boxVertices, SimpleVertex::Layout);

	Block::Object.SetInstanceBuffer(m_InstanceVBO);
}

void Level::SetupMatrices()
{
	m_Matrices.clear();
	m_Matrices.reserve(m_Blocks.size() - 1);

	for (Block& block : m_Blocks) {
		glm::vec3 start = block.Start;
		block.Start = glm::min(start, block.End);
		block.End = glm::max(start, block.End);

		// Extra factors are for help with that boxVertices are -1 to 1 instead of 0-1
		glm::mat4 matrix = glm::translate(glm::mat4(1.0f), 0.5f * (block.Start + block.End));
		matrix *= glm::mat4(block.Rotation);
		matrix = glm::scale(matrix, (block.End - block.Start) * 0.5f);

		m_Matrices.push_back(matrix);
	}
}

// Helper functions

static float sum(const glm::vec3& vec) {
	return vec.x + vec.y + vec.z;
}

static bool GetSeparatingPlane(const glm::vec3& rPos, const glm::vec3& plane, const OBB& box1, const OBB& box2) {
	using glm::abs;
	return ((abs(sum(rPos * plane)) >
		(abs(sum((box1.axisX * box1.halfSize[0]) * plane)) +
			abs(sum((box1.axisY * box1.halfSize[1]) * plane)) +
			abs(sum((box1.axisZ * box1.halfSize[2]) * plane)) +
			abs(sum((box2.axisX * box2.halfSize[0]) * plane)) +
			abs(sum((box2.axisY * box2.halfSize[1]) * plane)) +
			abs(sum((box2.axisZ * box2.halfSize[2]) * plane)))));
}

static bool CollisionCheck(const OBB& box1, const OBB& box2) {
	glm::vec3 rPos = box2.pos - box1.pos;

	return !(
		GetSeparatingPlane(rPos, box1.axisX, box1, box2) || GetSeparatingPlane(rPos, box1.axisY, box1, box2) ||
		GetSeparatingPlane(rPos, box1.axisZ, box1, box2) || GetSeparatingPlane(rPos, box2.axisX, box1, box2) ||
		GetSeparatingPlane(rPos, box2.axisY, box1, box2) || GetSeparatingPlane(rPos, box2.axisZ, box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisX, box2.axisX), box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisX, box2.axisY), box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisX, box2.axisZ), box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisY, box2.axisX), box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisY, box2.axisY), box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisY, box2.axisZ), box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisZ, box2.axisX), box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisZ, box2.axisY), box1, box2) ||
		GetSeparatingPlane(rPos, glm::cross(box1.axisZ, box2.axisZ), box1, box2));
}

// Min function, but seeing which is larger is based on the absolute value
static float AbsMin(float a, float b) {
	return glm::abs(a) > glm::abs(b) ? b : a;
}

// Assumes camera is colliding with block
static void BounceOn(const Level::Block& block, GrapplingCamera& camera) {
	glm::vec3 min = {
		AbsMin(block.Start.x - camera.PhysicsPosition.x, block.End.x - camera.PhysicsPosition.x),
		AbsMin(block.Start.y - camera.PhysicsPosition.y, block.End.y - camera.PhysicsPosition.y),
		AbsMin(block.Start.z - camera.PhysicsPosition.z, block.End.z - camera.PhysicsPosition.z)
	};

	if (glm::abs(min.x) > glm::abs(min.y)) {
		if (glm::abs(min.z) > glm::abs(min.y)) {
			camera.PhysicsPosition.y += min.y;
			camera.Vel.y = glm::max(camera.Vel.y, 0.0f);
			camera.m_CanJump = true;
		}
		else {
			camera.PhysicsPosition.z += min.z;
			camera.Vel.z = glm::max(camera.Vel.z, 0.0f);
		}
	}
	else {
		if (glm::abs(min.z) > glm::abs(min.x)) {
			camera.PhysicsPosition.x += min.x;
			camera.Vel.x = glm::max(camera.Vel.x, 0.0f);
		}
		else {
			camera.PhysicsPosition.z += min.z;
			camera.Vel.z = glm::max(camera.Vel.z, 0.0f);
		}
	}

	camera.Position = { camera.PhysicsPosition.x, camera.PhysicsPosition.y + PHYSICSOFFSET, camera.PhysicsPosition.z };
}