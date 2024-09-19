#pragma once

#include "../application.h"

class ExampleApp : public Application
{
protected:
	void render() override;
	void resize(int width, int height) override;
	void destroy() override;
};

