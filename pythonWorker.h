#pragma once

#include<string>

void initPython(std::string projectDir);
void pythonRun(std::string command);
std::string pythonEval(std::string command);
void pythonSetGlobal(std::string name, std::string value);
