﻿#pragma once

namespace qhenki::graphics
{
	struct CommandList
	{
		sPtr<void> internal_state;
		//bool is_recording = false;
		//bool is_executing = false;
		//bool is_submitted = false;
	};
}
