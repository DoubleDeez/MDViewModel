#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"

#include "Util/MDViewModelEditorAssignment.h"


void UMDViewModelWidgetBlueprintExtension::PostLoad()
{
	Super::PostLoad();

	for (FMDViewModelEditorAssignment& Assignment : Assignments)
	{
		Assignment.Assignment.UpdateViewModelClassName();
	}
}
