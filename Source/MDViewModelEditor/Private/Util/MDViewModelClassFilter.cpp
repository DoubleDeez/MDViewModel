#include "Util/MDViewModelClassFilter.h"

#include "ViewModel/MDViewModelBase.h"
#include "ViewModel/MDViewModelBlueprintBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"

FMDViewModelClassFilter::FMDViewModelClassFilter(UMDViewModelProviderBase* Provider)
{
	ProviderSupportedViewModelClasses = TArray<FMDViewModelSupportedClass>{};

	if (IsValid(Provider))
	{
		Provider->GetSupportedViewModelClasses(ProviderSupportedViewModelClasses.GetValue());
		bAllowAbstract = !Provider->DoesCreateViewModels();
	}
}

FMDViewModelClassFilter::FMDViewModelClassFilter(bool bAllowAbstract)
	: bAllowAbstract(bAllowAbstract)
{
}

bool FMDViewModelClassFilter::IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (InClass == UMDViewModelBase::StaticClass() || InClass == UMDViewModelBlueprintBase::StaticClass() || !InClass->IsChildOf<UMDViewModelBase>())
	{
		return false;
	}

	const bool bHasValidFlags = !InClass->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated);
	if (bHasValidFlags && (bAllowAbstract || !InClass->HasAnyClassFlags(CLASS_Abstract)))
	{
		if (!ProviderSupportedViewModelClasses.IsSet())
		{
			return true;
		}

		for (const FMDViewModelSupportedClass& SupportedClass : ProviderSupportedViewModelClasses.GetValue())
		{
			if (SupportedClass.Class == InClass || (SupportedClass.bAllowChildClasses && InClass->IsChildOf(SupportedClass.Class)))
			{
				return true;
			}
		}
	}

	return false;
}

bool FMDViewModelClassFilter::IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs)
{
	if (!InUnloadedClassData->IsChildOf(UMDViewModelBase::StaticClass()))
	{
		return false;
	}

	const bool bHasValidFlags = !InUnloadedClassData->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated);
	if (bHasValidFlags && (bAllowAbstract || !InUnloadedClassData->HasAnyClassFlags(CLASS_Abstract)))
	{
		if (!ProviderSupportedViewModelClasses.IsSet())
		{
			return true;
		}

		for (const FMDViewModelSupportedClass& SupportedClass : ProviderSupportedViewModelClasses.GetValue())
		{
			if (SupportedClass.bAllowChildClasses && InUnloadedClassData->IsChildOf(SupportedClass.Class))
			{
				return true;
			}
		}
	}

	return false;
}
