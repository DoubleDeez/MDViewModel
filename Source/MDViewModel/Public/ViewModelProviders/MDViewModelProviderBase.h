#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"

struct FMDViewModelAssignmentData;
class UGameInstance;
struct FMDViewModelAssignment;
class UUserWidget;
class UMDViewModelBase;

struct FMDViewModelSupportedClass
{
	TSubclassOf<UMDViewModelBase> Class;

	bool bAllowChildClasses = true;
};

/**
 * Abstract base class for creating custom view model providers
 *
 * A View Model Provider automatically assigns view models to a passed-in user widget
 */
class MDVIEWMODEL_API FMDViewModelProviderBase : public TSharedFromThis<FMDViewModelProviderBase>
{
public:
	virtual ~FMDViewModelProviderBase() {}

	// An opportunity to perform game-based initialization, bind to delegates, etc
	virtual void InitializeProvider(UGameInstance* GameInstance) {}

	// Perform the actual assignment of the view model
	virtual UMDViewModelBase* AssignViewModel(UUserWidget& Widget, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data) = 0;

#if WITH_EDITOR
	virtual void GetSupportedViewModelClasses(TArray<FMDViewModelSupportedClass>& OutViewModelClasses) = 0;
	virtual FText GetDisplayName() const = 0;
	virtual FText GetDescription() const = 0;

	virtual UScriptStruct* GetProviderSettingsStruct() const { return nullptr; }
#endif

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewModelUpdated, TSubclassOf<UMDViewModelBase>);
	FOnViewModelUpdated OnViewModelUpdated;

protected:
	// Call this when you need all assigned widgets to know that a view model has updated
	void BroadcastViewModelUpdated(TSubclassOf<UMDViewModelBase> ViewModelClass) const;
};
