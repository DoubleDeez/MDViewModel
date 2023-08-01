#pragma once

#include "MDViewModelAssignment.h"


// Each assignment on a widget needs to be individually tracked, this struct can serve as a key for it
struct MDVIEWMODEL_API FMDVMAssignmentWidgetKey
{
	FMDViewModelAssignment Assignment;

	TWeakObjectPtr<UUserWidget> WidgetPtr;

	bool operator==(const FMDVMAssignmentWidgetKey& Other) const;

	bool operator!=(const FMDVMAssignmentWidgetKey& Other) const
	{
		return !(*this == Other);
	}
};

inline uint32 GetTypeHash(const FMDVMAssignmentWidgetKey& Key)
{
	return HashCombine(GetTypeHash(Key.Assignment), GetTypeHash(Key.WidgetPtr));
}
