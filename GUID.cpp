#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

class UUIDEx {
#ifdef UNICODE
    typedef ::RPC_WSTR RPC_STR;
#else
    typedef ::RPC_CSTR RPC_STR;
#endif // UNICODE

public:
    inline UUIDEx()
        : mStatus(::UuidCreate(&mUUID)),
        mIsGlobal(mStatus != RPC_S_UUID_LOCAL_ONLY) {}

    inline ~UUIDEx() {
        if (mUUIDString && IsGood()) {
            ::RpcStringFree(&mUUIDString);
        }
    }

    inline bool IsGlobal() const {
        return mIsGlobal;
    }

    inline const RPC_STR& GetString() {
        if (!mUUIDString && IsGood()) {
            mStatus = ::UuidToString(&mUUID, &mUUIDString);
            mUUIDStringTryCreate = true;
        }

        return IsGood() ? mUUIDString : mUUIDStringDefault;
    }

    inline const ::UUID& Get() const {
        return IsGood() ? mUUID : mUUIDDefault;
    }

private:
    inline bool IsGood() const {
        if (mUUIDStringTryCreate) {
            return mStatus != RPC_S_OUT_OF_MEMORY;
        } else {
            if (mIsGlobal) {
                return true;
            } else {
                return mStatus != RPC_S_UUID_NO_ADDRESS;
            }
        }
    }

    ::UUID mUUID {};
    static inline ::UUID mUUIDDefault {};

    RPC_STR mUUIDString = nullptr;
    static inline RPC_STR mUUIDStringDefault = (RPC_STR)TEXT("");

    ::RPC_STATUS mStatus = 0;
    bool mIsGlobal = true;
    bool mUUIDStringTryCreate = false;
};
