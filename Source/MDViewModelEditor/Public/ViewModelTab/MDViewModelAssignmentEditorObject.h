#pragma once

#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "MDViewModelAssignmentEditorObject.generated.h"

class UMDViewModelBase;
class UWidgetBlueprint;

/**
 * Editor-time object used a proxy to display the settings when assigning a view model to a widget
 */
UCLASS(CollapseCategories)
class MDVIEWMODELEDITOR_API UMDViewModelAssignmentEditorObject : public UObject
{
	GENERATED_BODY()

public:
	void PopulateFromAssignment(const FMDViewModelEditorAssignment& Assignment, UWidgetBlueprint* WidgetBlueprint);

	FMDViewModelEditorAssignment CreateAssignment() const;

	// How will the viewmodel be provided to this widget?
	UPROPERTY(EditAnywhere, Category = "View Model")
	FGameplayTag ViewModelProvider;

	// Settings for the specified view model provider
	UPROPERTY(EditAnywhere, NoClear, Category = "View Model", meta = (StructTypeConst))
	FInstancedStruct ProviderSettings;

	UPROPERTY(EditAnywhere, Category = "View Model", meta = (InlineEditConditionToggle))
	bool bOverrideName = false;

	// A name to give the selected viewmodel. Must be unique for the selected viewmodel class for this widget.
	UPROPERTY(EditAnywhere, Category = "View Model", meta = (EditCondition = "bOverrideName"))
	FName ViewModelInstanceName = MDViewModelUtils::DefaultViewModelName;

	UPROPERTY(EditAnywhere, Category = "View Model")
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Settings for the specified view model class
	UPROPERTY(EditAnywhere, NoClear, Category = "View Model", meta = (StructTypeConst))
	FInstancedStruct ViewModelSettings;

};
