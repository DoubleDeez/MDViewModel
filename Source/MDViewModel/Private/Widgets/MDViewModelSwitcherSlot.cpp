#include "Widgets/MDViewModelSwitcherSlot.h"

#include "Components/Widget.h"

void UMDViewModelSwitcherSlot::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	ViewModelAssignment.OnGetWidgetClass.BindUObject(this, &UMDViewModelSwitcherSlot::GetContentWidgetClass);
#endif
}

#if WITH_EDITOR
UClass* UMDViewModelSwitcherSlot::GetContentWidgetClass() const
{
	return IsValid(Content) ? Content->GetClass() : nullptr;
}
#endif
