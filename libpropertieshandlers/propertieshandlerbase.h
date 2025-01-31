//
//  propertieshandlerbase.h
//  libpropertieshandlers
//
//  Created by Igor Korot on 8/18/20.
//  Copyright © 2020 Igor Korot. All rights reserved.
//

#ifndef propertieshandlerbase_h
#define propertieshandlerbase_h

class WXEXPORT PropertiesHandler
{
public:
    PropertiesHandler() { }
    virtual ~PropertiesHandler() { }
    virtual void EditProperies(wxNotebook *parent) = 0;
    virtual int GetProperties(std::vector<std::wstring> &errors) = 0;
    virtual bool IsLogOnly() const { return false; };
    virtual const std::wstring &GetCommand() const { return L""; };
};



#endif /* propertieshandlerbase_h */
