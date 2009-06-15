#include "Parameters.h"

namespace Torch
{

// Constructor
Parameters::Parameters()
	: 	m_parameters(new VariableCollector())
{
}

// Destructor
Parameters::~Parameters()
{
	delete m_parameters;
}

bool Parameters::addI(const char* name, int init_value, const char* help)
{
	return m_parameters->addI(name, init_value, help);
}
bool Parameters::addF(const char* name, float init_value, const char* help)
{
	return m_parameters->addF(name, init_value, help);
}
bool Parameters::addD(const char* name, double init_value, const char* help)
{
	return m_parameters->addD(name, init_value, help);
}
bool Parameters::addIarray(const char* name, const int n_values, const int init_value, const char* help)
{
	return m_parameters->addIarray(name, n_values, init_value, help);
}
bool Parameters::addFarray(const char* name, const int n_values, const float init_value, const char* help)
{
	return m_parameters->addFarray(name, n_values, init_value, help);
}
bool Parameters::addDarray(const char* name, const int n_values, const double init_value, const char* help)
{
	return m_parameters->addDarray(name, n_values, init_value, help);
}

bool Parameters::setI(const char* name, int new_value)
{
	if (m_parameters->setI(name, new_value) == true)
	{
		parameterChanged(name);
		return true;
	}
	return false;
}
bool Parameters::setF(const char* name, float new_value)
{
	if (m_parameters->setF(name, new_value) == true)
	{
		parameterChanged(name);
		return true;
	}
	return false;
}
bool Parameters::setD(const char* name, double new_value)
{
	if (m_parameters->setD(name, new_value) == true)
	{
		parameterChanged(name);
		return true;
	}
	return false;
}
bool Parameters::setIarray(const char* name, int n_values)
{
	return m_parameters->setIarray(name, n_values);
}
bool Parameters::setFarray(const char* name, int n_values)
{
	return m_parameters->setFarray(name, n_values);
}
bool Parameters::setDarray(const char* name, int n_values)
{
	return m_parameters->setDarray(name, n_values);
}

int Parameters::getI(const char* name, bool* ok)
{
	return m_parameters->getI(name, ok);
}
float Parameters::getF(const char* name, bool* ok)
{
	return m_parameters->getF(name, ok);
}
double Parameters::getD(const char* name, bool* ok)
{
	return m_parameters->getD(name, ok);
}
int* Parameters::getIarray(const char* name, bool* ok)
{
	return m_parameters->getIarray(name, ok);
}
float* Parameters::getFarray(const char* name, bool* ok)
{
	return m_parameters->getFarray(name, ok);
}
double* Parameters::getDarray(const char* name, bool* ok)
{
	return m_parameters->getDarray(name, ok);
}

void Parameters::print(const char *name)
{
	if(name != NULL) Torch::print("Parameters %s:\n", name);
	m_parameters->print();
}

// Loading/Saving the content from files (\emph{not the options}) - overriden
bool Parameters::loadFile(File& file)
{
   	return m_parameters->loadFile(file);
}

bool Parameters::saveFile(File& file) const
{
	return m_parameters->saveFile(file);
}

}

///////////////////////////////////////////////////////////
