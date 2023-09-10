#pragma once

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

inline uint32 GetTypeHash(const FMDVMAssignmentObjectKey& Key)
{
	return HashCombine(GetTypeHash(Key.Assignment), Key.ObjectPtr.GetWeakObjectPtr().GetWeakPtrTypeHash());
}

MDVIEWMODEL_API FCbWriter& operator<<(FCbWriter& Writer, const FMDVMAssignmentObjectKey& Key);
