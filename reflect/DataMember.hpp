#ifndef DATA_MEMBER_H
#define DATA_MEMBER_H

#include "TypeDescriptor.hpp"

#include <string>
#include <any>

namespace Reflect
{

	class DataMember
	{
	public:
		std::string GetName() const { return mName; }
		const TypeDescriptor *GetParent() const { return mParent; }
		const TypeDescriptor *GetType() const { return mType; }

		virtual void Set(std::any& objectRef, const std::any& value) = 0;
		virtual std::any Get(std::any const& object) = 0;

	protected:
		DataMember(const std::string &name, const TypeDescriptor *type, const TypeDescriptor *parent)
			: mName(name), mType(type), mParent(parent) {}

	private:
		std::string mName;                 
		const TypeDescriptor *mType;    // type of the data member
		const TypeDescriptor *mParent;  // type of the data member's class
	};

	template <typename Class, typename Type>
	class PtrDataMember : public DataMember
	{
	public:
		PtrDataMember(Type Class::*dataMemberPtr, const std::string name)
			: DataMember(name, Details::Resolve<Type>(), Details::Resolve<Class>()), mDataMemberPtr(dataMemberPtr) {}

		// void Set(std::anyRef objectRef, const std::any value) override
		// {
		// 	SetImpl(objectRef, value);  // use SFINAE
		// }

		void Set(std::any& objectRef, const std::any& value) override
		{
			SetImpl(objectRef, value, std::is_const<Type>());  // use tag dispatch
		}

		std::any Get(std::any const& object) override
		{
			return std::any_cast<Class>(&object)->*mDataMemberPtr;
		}

	private:
		Type Class::*mDataMemberPtr;

		////// use tag dispatch
		void SetImpl(std::any& object, const std::any& value, std::false_type)
		{
			Class *obj = std::any_cast<Class>(&object);  // pointers to members of base class can be used with derived class
			Type const *val = std::any_cast<Type>(&value);

			obj->*mDataMemberPtr = *val;
		}

		void SetImpl(std::any& object, const std::any& value, std::true_type)
		{
			//static_assert(false, "can't set const data member");
		}
	};

	// helper meta function to get info about functions passed as auto non type params (C++17)
	template <typename>
	struct FunctionHelper;

	template <typename Ret, typename... Args>
	struct FunctionHelper<Ret(Args...)>
	{
		using ReturnType = Ret;
		using ParamsTypes = std::tuple<Args...>;
	};

	template <typename Class, typename Ret, typename... Args>
	/*constexpr*/ FunctionHelper<Ret(/*Class, */Args...)> ToFunctionHelper(Ret(Class::*)(Args...));

	template <typename Class, typename Ret, typename... Args>
	/*constexpr*/ FunctionHelper<Ret(/*Class, */Args...)> ToFunctionHelper(Ret(Class::*)(Args...) const);

	template <typename Ret, typename... Args>
	/*constexpr*/ FunctionHelper<Ret(Args...)> ToFunctionHelper(Ret(*)(Args...));

	template <auto Setter, auto Getter, typename Class>
	class SetGetDataMember : public DataMember
	{
	private:
		using MemberType = Details::RawType<typename decltype(ToFunctionHelper(Getter))::ReturnType>;

	public:
		SetGetDataMember(const std::string name)
			: DataMember(name, Details::Resolve<MemberType>(), Details::Resolve<Class>()) {}

		void Set(std::any& objectRef, const std::any& value) override
		{
			Class *obj = std::any_cast<Class>(&objectRef);
			MemberType *val = std::any_cast<MemberType>(&value);

			if constexpr (std::is_member_function_pointer_v<decltype(Setter)>)
				(obj->*Setter)(*val);
			else
			{
				static_assert(std::is_function_v<std::remove_pointer_t<decltype(Setter)>>);

				Setter(*obj, *val);
			}

		}

		std::any Get(std::any const& object) override
		{
			Class *obj = std::any_cast<Class>(&object);

			if constexpr (std::is_member_function_pointer_v<decltype(Setter)>)
				return (obj->*Getter)();
			else
			{
				static_assert(std::is_function_v<std::remove_pointer_t<decltype(Getter)>>);

				return Getter(*obj);
			}
		}
	};

}  // namespace Reflect

#endif // DATA_MEMBER_H
