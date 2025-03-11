namespace qhenki::gfx
{
	enum BufferUsage : uint8_t
	{
		VERTEX = 1 << 0,
		INDEX = 1 << 1,
		UNIFORM = 1 << 2,
		STORAGE = 1 << 3,
		INDIRECT = 1 << 4,
		TRANSFER_SRC = 1 << 5,
		TRANSFER_DST = 1 << 6,
	};

	enum BufferVisibility : uint8_t
	{
		// Device local memory. Can be & with other visibilities to try to get BAR memory
		GPU = 0,
		// Host Visible: Written to by the CPU sequentially
		CPU_SEQUENTIAL = 1 << 0,
		// Host Visible: Written to by the CPU randomly. Cached randomly, try to avoid using this
		CPU_RANDOM = 1 << 1,
	};

    // Constant/Uniform buffers must follow D3D11 alignment rules! (equivalent to std140 GLSL)
	struct BufferDesc
	{
		uint64_t size = 0;
		BufferUsage usage;
		BufferVisibility visibility;
	};

	struct Buffer
	{
		BufferDesc desc;
		sPtr<void> internal_state;
	};
}
