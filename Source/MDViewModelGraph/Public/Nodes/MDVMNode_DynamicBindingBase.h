#pragma once

#include "K2Node_Event.h"
#include "MDVMNode_DynamicBindingBase.generated.h"

class UMDViewModelBase;

UCLASS(Abstract)
class MDVIEWMODELGRAPH_API UMDVMNode_DynamicBindingBase : public UK2Node_Event
{
	GENERATED_BODY()

public:
	virtual void AllocateDefaultPins() override;
	virtual void BeginDestroy() override;

	virtual FString GetFindReferenceSearchString() const override;
	
	// Class of the view model we're binding to
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Name of the view model we're binding to
	UPROPERTY()
	FName ViewModelName = NAME_None;

protected:
	FNodeTextCache CachedNodeTitle;

	virtual void OnAssignmentChanged();

private:
	void BindAssignmentChanges();
	void OnAssignmentNameChanged(TSubclassOf<UMDViewModelBase> VMClass, const FName& OldName, const FName& NewName);
	void OnAssignmentClassChanged(const FName& VMName, TSubclassOf<UMDViewModelBase> OldClass, TSubclassOf<UMDViewModelBase> NewClass);
	void UnbindAssignmentChanges();
};
