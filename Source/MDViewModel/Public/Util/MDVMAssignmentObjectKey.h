#pragma once

#include "Launch/Resources/Version.h"
#include "MDViewModelAssignment.h"
#include "UObject/WeakInterfacePtr.h"

class IMDViewModelRuntimeInterface;

// Each assignment on an object needs to be individually tracked, this struct can serve as a key for it
struct MDVIEWMODEL_API FMDVMAssignmentObjectKey
{
	FMDViewModelAssignment Assignment;

	// TODO - Make TInterfaceKey or TObjectKey accept IInterface
	TWeakInterfacePtr<IMDViewModelRuntimeInterface> ObjectPtr;

	bool operator==(const FMDVMAssignmentObjectKey& Other) const;

	bool operator!=(const FMDVMAssignmentObjectKey& Other) const
	{
		return !(*this == Other);
	}
};

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1
FORCEINLINE uint32 GetTypeHash(const FMDVMAssignmentObjectKey& Key)
{
	return HashCombine(GetTypeHash(Key.Assignment), GetTypeHash(Key.ObjectPtr.GetWeakObjectPtr()));
}
#else
FORCEINLINE uint32 GetTypeHash(const FMDVMAssignmentObjectKey& Key)
{
	return HashCombine(GetTypeHash(Key.Assignment), Key.ObjectPtr.GetWeakObjectPtr().GetWeakPtrTypeHash());
}
#endif

MDVIEWMODEL_API FCbWriter& operator<<(FCbWriter& Writer, const FMDVMAssignmentObjectKey& Key);
