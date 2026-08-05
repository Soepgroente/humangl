#include "Humanimation.hpp"

#include <stdexcept>
#include <iostream>

int main()
{
	try
	{
		Humanimation app;

		app.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "An error occured: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}