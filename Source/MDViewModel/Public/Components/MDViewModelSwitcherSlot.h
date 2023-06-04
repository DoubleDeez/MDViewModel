#pragma once

#include "Components/WidgetSwitcherSlot.h"
#include "Util/MDViewModelUtils.h"
#include "MDViewModelSwitcherSlot.generated.h"

/**
 *
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelSwitcherSlot : public UWidgetSwitcherSlot
{
	GENERATED_BODY()

public:
	bool DoesSupportChildViewModelClasses() const { return bSupportChildViewModelClasses; }

	const FName& GetViewModelName() const { return ViewModelName; }

protected:
	// Whether or not to switch to this widget for child viewmodel classes that it supports, otherwise requiring exact viewmodel-class matching.
	UPROPERTY(EditAnywhere, Category="Layout|Widget Switcher Slot")
	bool bSupportChildViewModelClasses = true;

	// What name to use to assign viewmodels to this widget
	UPROPERTY(EditAnywhere, Category="Layout|Widget Switcher Slot")
	FName ViewModelName = MDViewModelUtils::DefaultViewModelName;
};
