#pragma once

#include "K2Node_Event.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDVMNode_DynamicBindingBase.generated.h"

class UMDViewModelBase;

UCLASS(Abstract)
class MDVIEWMODELGRAPH_API UMDVMNode_DynamicBindingBase : public UK2Node_Event
{
	GENERATED_BODY()

public:
	virtual void ReconstructNode() override;
	virtual void AllocateDefaultPins() override;
	virtual void BeginDestroy() override;

	virtual FString GetFindReferenceSearchString() const override;

	virtual void PostLoad() override;

	FText GetViewModelClassName() const;

	UPROPERTY()
	FMDViewModelAssignmentReference Assignment;

protected:
	FNodeTextCache CachedNodeTitle;

	virtual void OnAssignmentChanged();

private:
	void BindAssignmentChanges();
	void OnAssignmentChanged(const FMDViewModelAssignmentReference& Old, const FMDViewModelAssignmentReference& New);
	void UnbindAssignmentChanges();

	void UpdateDeprecatedProperties();

	// Deprecated for Assignment
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;
	// Deprecated for Assignment
	UPROPERTY()
	FName ViewModelName = NAME_None;
};
