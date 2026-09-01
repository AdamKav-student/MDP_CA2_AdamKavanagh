// Adam Kavanagh - D00247069
#include "application.hpp"
#include <iostream>

int main()
{
	try
	{
		Application app;
		app.Run();
	}
	catch (std::runtime_error& e)
	{
		std::cout << e.what() << std::endl;
	}
	
}