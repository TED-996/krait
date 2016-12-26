#pragma once

#include<string>
#include<map>
#include"request.h"

void pythonInit(std::string projectDir);
void pythonReset(std::string projectDir);
void pythonRun(std::string command);
std::string pythonEval(std::string command);

void pythonSetGlobal(std::string name, std::string value);
void pythonSetGlobal(std::string name, std::map<std::string, std::string> value);

void pythonSetGlobalRequest(std::string name, Request value);

std::string pythonGetGlobalStr(std::string name);
std::map<std::string, std::string> pythonGetGlobalMap(std::string name);
