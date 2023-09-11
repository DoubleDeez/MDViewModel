#pragma once

#include "Extensions/UserWidgetExtension.h"
#include "Interfaces/MDViewModelRuntimeInterface.h"
#include "Templates/SubclassOf.h"
#include "UObject/Package.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDViewModelWidgetExtension.generated.h"

struct FGameplayTag;
struct FInstancedStruct;
class UMDViewModelBase;

/**
 * A widget extension to track a widget's view models
 */
UCLASS(BlueprintType)
class MDVIEWMODEL_API UMDViewModelWidgetExtension : public UUserWidgetExtension, public IMDViewModelRuntimeInterface
{
	GENERATED_BODY()

public:
	virtual void Construct() override;
	virtual void Destruct() override;
	virtual void BeginDestroy() override;

	static UMDViewModelWidgetExtension* GetOrCreate(UUserWidget* Widget);
	
	virtual UObject* GetOwningObject() const override;

	virtual UGameInstance* ResolveGameInstance() const override;
	virtual UWorld* ResolveWorld() const override;
	virtual ULocalPlayer* ResolveOwningLocalPlayer() const override;
	virtual APlayerController* ResolveOwningPlayer() const override;

protected:
	virtual TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& GetViewModels() override;

private:
	UPROPERTY(Transient)
	TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>> ViewModels;
	
};
