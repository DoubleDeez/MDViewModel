#include "BlueprintExtensions/MDViewModelSupportedBlueprintExtension.h"

void UMDViewModelSupportedBlueprintExtension::BeginDestroy()
{
	Super::BeginDestroy();

	// Work around issue where ~FInstancedStruct would crash when calling DestroyStruct on partially destroyed UserDefinedStructs
	Assignments.Empty();
}

void UMDViewModelSupportedBlueprintExtension::PostLoad()
{
	Super::PostLoad();

	for (FMDViewModelEditorAssignment& Assignment : Assignments)
	{
		Assignment.Assignment.UpdateViewModelClassName();
	}
}
