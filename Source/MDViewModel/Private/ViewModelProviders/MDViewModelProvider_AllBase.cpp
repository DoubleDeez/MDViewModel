#include "ViewModelProviders/MDViewModelProvider_AllBase.h"

#include "EditorClassUtils.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "UObject/UObjectHash.h"
#include "ViewModel/MDViewModelBase.h"

#if WITH_EDITOR
void FMDViewModelProvider_AllBase::GetSupportedViewModelClasses(TArray<FMDViewModelSupportedClass>& OutViewModelClasses)
{
	OutViewModelClasses.Add({ UMDViewModelBase::StaticClass() });
}
#endif
