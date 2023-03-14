#include <Rpc.h>
#pragma comment(lib, "Rpcrt4.lib")

namespace Utility {
	class GUID {
#if defined(UNICODE) || defined(_UNICODE)
		using RPC_STR = ::RPC_WSTR;
#else
		using RPC_STR = ::RPC_CSTR;
#endif // defined(UNICODE) || defined(_UNICODE)

	public:
		GUID()
			: mStatus(::UuidCreate(&mUUID)),
			mIsGlobal(mStatus != RPC_S_UUID_LOCAL_ONLY),
			mIsGood(mStatus != RPC_S_UUID_NO_ADDRESS) {
		}

		~GUID() {
			Uninitialize();
		}

		GUID(const GUID& guid) {
			*this = guid;
		}

		GUID& operator=(const GUID& right) {
			Uninitialize();

			mUUID = right.mUUID;
			mStatus = RPC_S_OK;
			mIsGood = right.mIsGood;
			mIsGlobal = right.mIsGlobal;

			return *this;
		}

		bool IsGlobal() const {
			return mIsGlobal;
		}

		bool IsGood() const {
			return mIsGood;
		}

		void SetUUID(const ::UUID& uuid) {
			Uninitialize();

			mUUID = uuid;
			mStatus = RPC_S_OK;
			mIsGood = true;
			mIsGlobal = false;
		}

		void SetUUID(RPC_CSTR uuidStr) {
			Uninitialize();

			mStatus = ::UuidFromString(uuidStr, &mUUID);
			mIsGood = mStatus == RPC_S_OK;
			mIsGlobal = false;
		}

		const RPC_STR& GetStr() {
			if (!mUUIDString) {
				mStatus = ::UuidToString(&mUUID, &mUUIDString);
			}

			return mStatus == RPC_S_OK ? mUUIDString : mUUIDStringDefault;
		}

		const ::UUID& GetUUID() const {
			return mIsGood ? mUUID : mUUIDDefault;
		}

		::RPC_STATUS GetStatus() const {
			return mStatus;
		}

	private:
		void Uninitialize() {
			if (mUUIDString) {
				::RpcStringFree(&mUUIDString);
				mUUIDString = nullptr;
			}
		}

		::UUID mUUID = GUID_NULL;
		static inline ::UUID mUUIDDefault {};

		RPC_STR mUUIDString {};
		static inline RPC_STR mUUIDStringDefault = (RPC_STR)TEXT("");

		::RPC_STATUS mStatus {};
		bool mIsGood {};
		bool mIsGlobal {};
	};
}
