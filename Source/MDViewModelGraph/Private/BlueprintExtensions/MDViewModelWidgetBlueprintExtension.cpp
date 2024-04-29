#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"

#include "Util/MDViewModelEditorAssignment.h"


void UMDViewModelWidgetBlueprintExtension::BeginDestroy()
{
	Super::BeginDestroy();

	// Work around issue where ~FInstancedStruct would crash when calling DestroyStruct on partially destroyed UserDefinedStructs
	Assignments.Empty();
}

void UMDViewModelWidgetBlueprintExtension::PostLoad()
{
	Super::PostLoad();

	for (FMDViewModelEditorAssignment& Assignment : Assignments)
	{
		Assignment.Assignment.UpdateViewModelClassName();
	}
}
