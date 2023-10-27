#include "Util/MDViewModelAssignment.h"

#include "Serialization/CompactBinaryWriter.h"
#include "ViewModel/MDViewModelBase.h"


#if WITH_EDITORONLY_DATA
void FMDViewModelAssignment::UpdateViewModelClassName()
{
	if (::IsValid(ViewModelClass))
	{
		ViewModelClassName = ViewModelClass->GetFName();
	}
}
#endif

bool FMDViewModelAssignment::IsValid() const
{
	return ::IsValid(ViewModelClass) && ProviderTag.IsValid() && ViewModelName.IsValid();
}

void FMDViewModelAssignment::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsSaving() && IsValid())
	{
		// Save broad and specific references
		Ar.MarkSearchableName(StaticStruct(), ViewModelClass->GetFName());
		Ar.MarkSearchableName(StaticStruct(), *FString::Printf(TEXT("%s.%s"), *ViewModelClass->GetFName().ToString(), *ViewModelName.ToString()));
		Ar.MarkSearchableName(StaticStruct(), *FString::Printf(TEXT("%s.%s.%s"), *ViewModelClass->GetFName().ToString(), *ViewModelName.ToString(), *ProviderTag.ToString()));
	}
}

bool FMDViewModelAssignment::operator==(const FMDViewModelAssignment& Other) const
{
#if WITH_EDITORONLY_DATA
	if (ViewModelClassName != Other.ViewModelClassName)
	{
		return false;
	}
#endif

	return ViewModelClass == Other.ViewModelClass && ProviderTag == Other.ProviderTag && ViewModelName == Other.ViewModelName;
}

FCbWriter& operator<<(FCbWriter& Writer, const FMDViewModelAssignment& Assignment)
{
	Writer.BeginObject();
	Writer << "Class" << GetNameSafe(Assignment.ViewModelClass);
	Writer << "Provider" << Assignment.ProviderTag.GetTagName();
	Writer << "Name" << Assignment.ViewModelName;
#if WITH_EDITORONLY_DATA
	Writer << "ClassPath" << Assignment.ViewModelClassName;
#endif
	Writer.EndObject();
	return Writer;
}
