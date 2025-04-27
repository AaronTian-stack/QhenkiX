namespace qhenki::gfx
{
	enum class BufferUsage : uint8_t
	{
		VERTEX = 1 << 0,
		INDEX = 1 << 1,
		UNIFORM = 1 << 2, // CONSTANT
		STORAGE = 1 << 3, // UAV
		INDIRECT = 1 << 4,
		COPY_SRC = 1 << 5,
		COPY_DST = 1 << 6,
	};

	constexpr BufferUsage operator|(BufferUsage lhs, BufferUsage rhs) {
		using Underlying = std::underlying_type_t<BufferUsage>;
		return static_cast<BufferUsage>(
			static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs)
			);
	}

	constexpr bool operator&(BufferUsage lhs, BufferUsage rhs) {
		using Underlying = std::underlying_type_t<BufferUsage>;
		return (static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs)) != 0;
	}

	enum BufferVisibility : uint8_t
	{
		// Device local memory. Can be & with other visibilities to try to get BAR memory
		GPU = 0,
		// Host Visible: Written to by the CPU sequentially
		CPU_SEQUENTIAL = 1 << 0,
		// Host Visible: Written to by the CPU randomly. Cached randomly, try to avoid using this
		CPU_RANDOM = 1 << 1,
	};

    // Constant/Uniform buffers must follow D3D11 alignment rules (equivalent to std140 GLSL)
	struct BufferDesc
	{
		uint64_t size = 0;
		BufferUsage usage; // Only used in Vulkan
		BufferVisibility visibility;
	};

	struct Buffer
	{
		BufferDesc desc;
		sPtr<void> internal_state;
	};
}
