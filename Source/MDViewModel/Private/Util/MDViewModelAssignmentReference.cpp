#include "Util/MDViewModelAssignmentReference.h"

#include "Blueprint/UserWidget.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"

UMDViewModelBase* FMDViewModelAssignmentReference::ResolveViewModelAssignment(const UObject* Object) const
{
	if (const UUserWidget* Widget = Cast<UUserWidget>(Object))
	{
		const UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			// SoftClassPtr doesn't need loading since it will be loaded if the view model exists
			return Extension->GetViewModel(ViewModelClass.Get(), ViewModelName);
		}
	}
	else
	{
		// TODO - Actor View Models
	}

	return nullptr;
}

bool FMDViewModelAssignmentReference::IsAssignmentValid() const
{
	return !ViewModelClass.IsNull() && ViewModelName != NAME_None;
}

FMDViewModelAssignmentReference& FMDViewModelAssignmentReference::operator=(const FMDViewModelAssignmentReference& Other)
{
	ViewModelClass = Other.ViewModelClass;
	ViewModelName = Other.ViewModelName;

#if WITH_EDITOR
	OnGetWidgetClass = Other.OnGetWidgetClass;
#endif

	return *this;
}
