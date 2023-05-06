#include "Util/MDViewModelFunctionLibrary.h"

#include "Blueprint/UserWidget.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"

UMDViewModelBase* UMDViewModelFunctionLibrary::AssignViewModel(UUserWidget* Widget, UMDViewModelBase* ViewModel, FName ViewModelName)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		return Extension->AssignViewModel(ViewModel, ViewModelName);
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelFunctionLibrary::AssignViewModelOfClass(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		return Extension->AssignViewModelOfClass(ViewModelClass, ViewModelName);
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelFunctionLibrary::GetViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(Widget))
	{
		const UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			return Extension->GetViewModel(ViewModelClass, ViewModelName);
		}
	}

	return nullptr;
}
