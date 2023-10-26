#include "BlueprintExtensions/MDViewModelSupportedBlueprintExtension.h"

void UMDViewModelSupportedBlueprintExtension::PostLoad()
{
	Super::PostLoad();

	for (FMDViewModelEditorAssignment& Assignment : Assignments)
	{
		Assignment.Assignment.UpdateViewModelClassName();
	}
}
