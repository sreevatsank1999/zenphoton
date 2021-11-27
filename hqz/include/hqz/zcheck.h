
#pragma once
#include "rapidjson/document.h"
#include <sstream>

class ZCheck
{
private:
    typedef rapidjson::Value Value;

    static inline std::ostringstream mError = std::ostringstream();

public:
    
    static const char *errorText();
    static bool hasError();

    // Data checks
    static bool checkTuple(const Value &v, const char *noun, unsigned expected);
    static int checkInteger(const Value &v, const char *noun);
    static int checkBool(const Value &v, const char *noun);
    static double checkNumber(const Value &v, const char *noun);
    static bool checkMaterialID(const Value &v, const Value &mMaterials);
    static bool checkMaterialValue(int index, const Value &mMaterials);
    static bool checkStopCondition(double mRayLimit, double mTimeLimit);
    static bool checkLightPower(double mLightPower);
};
