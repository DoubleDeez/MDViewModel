#pragma once

#include "K2Node_Event.h"
#include "MDVMNode_DynamicBindingBase.generated.h"

class UMDViewModelBase;

UCLASS(Abstract)
class MDVIEWMODELGRAPH_API UMDVMNode_DynamicBindingBase : public UK2Node_Event
{
	GENERATED_BODY()

public:
	virtual void ReconstructNode() override;
	virtual void BeginDestroy() override;
	
	// Class of the view model we're binding to
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Name of the view model we're binding to
	UPROPERTY()
	FName ViewModelName = NAME_None;

protected:
	FNodeTextCache CachedNodeTitle;

private:
	void BindAssignmentNameChanged();
	void OnAssignmentNameChanged(TSubclassOf<UMDViewModelBase> VMClass, const FName& OldName, const FName& NewName);
	void UnbindAssignmentNameChanged();
};
