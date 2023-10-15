#include "BlueprintExtensions/MDViewModelActorBlueprintExtension.h"

void UMDViewModelActorBlueprintExtension::PostLoad()
{
	Super::PostLoad();

	for (FMDViewModelEditorAssignment& Assignment : Assignments)
	{
		Assignment.Assignment.UpdateViewModelClassName();
	}
}
