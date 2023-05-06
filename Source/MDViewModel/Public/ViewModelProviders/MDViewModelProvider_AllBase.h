#pragma once

#include "CoreMinimal.h"
#include "MDViewModelProviderBase.h"

/**
 * A view model provider base class that supports all view model classes by default
 */
class MDVIEWMODEL_API FMDViewModelProvider_AllBase : public FMDViewModelProviderBase
{
public:
#if WITH_EDITOR
	virtual void GetSupportedViewModelClasses(TArray<FMDViewModelSupportedClass>& OutViewModelClasses) override;
#endif
};
