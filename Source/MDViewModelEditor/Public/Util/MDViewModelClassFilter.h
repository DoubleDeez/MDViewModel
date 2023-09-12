#pragma once

#include "ClassViewerFilter.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"

class UMDViewModelProviderBase;

class MDVIEWMODELEDITOR_API FMDViewModelClassFilter : public IClassViewerFilter
{
public:
	explicit FMDViewModelClassFilter(UMDViewModelProviderBase* Provider);
	explicit FMDViewModelClassFilter(bool bAllowAbstract);

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs) override;
	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const class IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs) override;

private:
	bool bAllowAbstract = false;
	TOptional<TArray<FMDViewModelSupportedClass>> ProviderSupportedViewModelClasses;
};
