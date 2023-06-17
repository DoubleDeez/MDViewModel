#pragma once

#include "Components/WidgetSwitcherSlot.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelUtils.h"
#include "MDViewModelSwitcherSlot.generated.h"

/**
 *
 */
UCLASS(meta = (DisplayName = "Widget Switcher Slot (View Model)"))
class MDVIEWMODEL_API UMDViewModelSwitcherSlot : public UWidgetSwitcherSlot
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;

	bool DoesSupportChildViewModelClasses() const { return bSupportChildViewModelClasses; }

	const FMDViewModelAssignmentReference& GetViewModelAssignment() const { return ViewModelAssignment; }

protected:
	// Whether or not to switch to this widget for child viewmodel classes that it supports, otherwise requiring exact viewmodel-class matching.
	UPROPERTY(EditAnywhere, Category="Layout|Widget Switcher Slot")
	bool bSupportChildViewModelClasses = false;

	// Which assignment to use to assign view models to this widget
	UPROPERTY(EditAnywhere, Category="Layout|Widget Switcher Slot")
	FMDViewModelAssignmentReference ViewModelAssignment;

private:
#if WITH_EDITOR
	UClass* GetContentWidgetClass() const;
#endif
};
