#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

class UUID_RAII
{
#ifdef UNICODE
	typedef ::RPC_WSTR RPC_STR;
#else
	typedef ::RPC_CSTR RPC_STR;
#endif // UNICODE

public:
	UUID_RAII() {
		if (::UuidCreate(&mUUID) == RPC_S_UUID_NO_ADDRESS) {
			return;
		}

		mStatus = ::UuidToString(&mUUID, &mUUIDString);
	}

	~UUID_RAII() {
		if (!mStatus) {
			::RpcStringFree(&mUUIDString);
		}
	}

	inline const RPC_STR& GetUUIDString() const {
		return mUUIDString;
	}

	inline const ::UUID& GetUUID() const {
		return mUUID;
	}

private:
	::UUID mUUID{};
	RPC_STR mUUIDString = nullptr;
	::RPC_STATUS mStatus = 0;
};
