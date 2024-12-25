#define SDL_MAIN_HANDLED

#include "exampleapp.h"

int main()
{
    // Simple example application for the QhenkiX Game Framework.

	ExampleApp app;
	app.run(vendetta::D3D11);

    return 0;
}
