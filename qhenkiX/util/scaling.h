#pragma once
#include <DirectXMath.h>

using namespace DirectX;

class Scaling
{
public:
	virtual XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) = 0;
};

class ScalingNone : public Scaling
{
public:
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		return { target_width, target_height };
	}
};

class ScalingFit : public Scaling
{
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		float source_aspect = source_width / source_height;
		float target_aspect = target_width / target_height;
		float scale = target_aspect > source_aspect ? target_height / source_height : target_width / source_width;
		return { source_width * scale, source_height * scale };
	}
};

class ScalingContain : public Scaling
{
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		float source_aspect = source_width / source_height;
		float target_aspect = target_width / target_height;
		float scale = target_aspect < source_aspect ? target_height / source_height : target_width / source_width;
		return { source_width * scale, source_height * scale };
	}
};

class ScalingFill : public Scaling
{
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		float source_aspect = source_width / source_height;
		float target_aspect = target_width / target_height;
		float scale = target_aspect > source_aspect ? target_height / source_height : target_width / source_width;
		return { source_width * scale, source_height * scale };
	}
};

class ScalingFillX : public Scaling
{
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		float scale = target_width / source_width;
		return { source_width * scale, source_height * scale };
	}
};

class ScalingFillY : public Scaling
{
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		float scale = target_height / source_height;
		return { source_width * scale, source_height * scale };
	}
};

class ScalingStretch : public Scaling
{
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		return { target_width, target_height };
	}
};

class ScalingStretchX : public Scaling
{
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		return { target_width, source_height };
	}
};

class ScalingStretchY : public Scaling
{
	XMFLOAT2 apply(float source_width, float source_height, float target_width, float target_height) override
	{
		return { source_width, target_height };
	}
};

struct ScalingStatic
{
	static ScalingNone none;
	static ScalingFit fit;
	static ScalingContain contain;
	static ScalingFill fill;
	static ScalingFillX fill_x;
	static ScalingFillY fill_y;
	static ScalingStretch stretch;
	static ScalingStretchX stretch_x;
	static ScalingStretchY stretch_y;
};