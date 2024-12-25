#pragma once
#include <string>
#include <Wbemidl.h>
#include <comdef.h>

std::string BSTRToString(BSTR bstr);
std::string GetWMIPropertyFromObject(IWbemClassObject* pclsObj, const std::wstring& property);
std::string ParseWMIDate(const std::string& wmiDate);
HRESULT InitializeCOMAndSecurity();
HRESULT InitializeWMI();
