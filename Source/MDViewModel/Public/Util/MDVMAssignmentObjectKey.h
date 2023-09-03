#pragma once

#include "MDViewModelAssignment.h"


// Each assignment on an object needs to be individually tracked, this struct can serve as a key for it
struct MDVIEWMODEL_API FMDVMAssignmentObjectKey
{
	FMDViewModelAssignment Assignment;

	TWeakObjectPtr<UObject> ObjectPtr;

	bool operator==(const FMDVMAssignmentObjectKey& Other) const;

	bool operator!=(const FMDVMAssignmentObjectKey& Other) const
	{
		return !(*this == Other);
	}
};

inline uint32 GetTypeHash(const FMDVMAssignmentObjectKey& Key)
{
	return HashCombine(GetTypeHash(Key.Assignment), GetTypeHash(Key.ObjectPtr));
}

MDVIEWMODEL_API FCbWriter& operator<<(FCbWriter& Writer, const FMDVMAssignmentObjectKey& Key);
