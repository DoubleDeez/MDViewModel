#include "MDViewModelModule.h"

#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Util/MDViewModelAssignment.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetExtensions/MDViewModelWidgetClassExtension.h"


void FMDViewModelModule::GetViewModelClassesForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TSet<TSubclassOf<UMDViewModelBase>>& OutViewModelClasses) const
{
	if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
		{
			ClassExtension->GetViewModelClasses(OutViewModelClasses);
		}
	}
}

void FMDViewModelModule::GetViewModelAssignmentsForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const
{
	if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
		{
			OutViewModelAssignments.Append(ClassExtension->GetAssignments());
		}
	}
}

void FMDViewModelModule::SearchViewModelAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UUserWidget> WidgetClass, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName) const
{
	if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass))
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
		if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
		{
			ClassExtension->SearchAssignments(OutViewModelAssignments, ViewModelClass, ProviderTag, ViewModelName);
		}
	}
}

bool FMDViewModelModule::DoesClassOrSuperClassHaveAssignments(TSubclassOf<UUserWidget> WidgetClass) const
{
	UClass* Class = WidgetClass;
	while (Class != nullptr)
	{
		if (UWidgetBlueprintGeneratedClass* WBGC = Cast<UWidgetBlueprintGeneratedClass>(Class))
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 2
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>())
#else
			if (const UMDViewModelWidgetClassExtension* ClassExtension = WBGC->GetExtension<UMDViewModelWidgetClassExtension>(false))
#endif
			{
				if (ClassExtension->HasAssignments())
				{
					return true;
				}
			}
		}

		Class = Class->GetSuperClass();
	}

	return false;
}

IMPLEMENT_MODULE(FMDViewModelModule, MDViewModel)