
#include "zcheck.h"

const char* ZCheck::errorText() {
    return mError.str().c_str(); 
}
bool ZCheck::hasError() {
    return !mError.str().empty(); 
}

int ZCheck::checkInteger(const Value &v, const char *noun)
{
    /*
     * Convenience method to evaluate an integer. If it's Null, returns zero quietly.
     * If it's a valid integer, returns it quietly. Otherwise returns zero and logs an error.
     */

    if (v.IsNull())
        return 0;

    if (v.IsInt())
        return v.GetInt();

    mError << "'" << noun << "' expected an integer value\n";
    return 0;
}

double ZCheck::checkNumber(const Value &v, const char *noun)
{
    /*
     * Convenience method to evaluate a number. If it's Null, returns zero quietly.
     * If it's a valid number, returns it quietly. Otherwise returns zero and logs an error.
     */

    if (v.IsNull())
        return 0;

    if (v.IsNumber())
        return v.GetDouble();

    mError << "'" << noun << "' expected a number value\n";
    return 0;
}

bool ZCheck::checkTuple(const Value &v, const char *noun, unsigned expected)
{
    /*
     * A quick way to check for a tuple with an expected length, and set
     * an error if there's any problem. Returns true on success.
     */

    if (!v.IsArray() || v.Size() < expected) {
        mError << "'" << noun << "' expected an array with at least "
            << expected << " item" << (expected == 1 ? "" : "s") << "\n";
        return false;
    }

    return true;
}

bool ZCheck::checkMaterialID(const Value &v, const Value &mMaterials)
{
    // Check for a valid material ID.

    if (!v.IsUint()) {
        mError << "Material ID must be an unsigned integer\n";
        return false;
    }

    if (v.GetUint() >= mMaterials.Size()) {
        mError << "Material ID (" << v.GetUint() << ") out of range\n";
        return false;
    }

    return true;
}

bool ZCheck::checkMaterialValue(int index, const Value &mMaterials)
{
    const Value &v = mMaterials[index];

    if (!v.IsArray()) {
        mError << "Material #" << index << " is not an array\n";
        return false;
    }

    bool result = true;
    for (unsigned i = 0, e = v.Size(); i != e; ++i) {
        const Value& outcome = v[i];

        if (!outcome.IsArray() || outcome.Size() < 1 || !outcome[0u].IsNumber()) {
            mError << "Material #" << index << " outcome #" << i << "is not an array starting with a number\n";
            result = false;
        }
    }

    return result;
}

bool ZCheck::checkStopCondition(double mRayLimit, double mTimeLimit)
{
    if (mRayLimit <= 0.0 && mTimeLimit <= 0.0) {
            mError << "No stopping conditions set. Expected a ray limit and/or time limit.\n";
            return false;
        }

    return true;
}

bool ZCheck::checkLightPower(double mLightPower)
{
    if (mLightPower <= 0.0) {
        mError << "Total light power (" << mLightPower << ") must be positive.\n";
        return false;
    }

    return true;
}