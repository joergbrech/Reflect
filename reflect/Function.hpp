#ifndef MEMBER_FUNCTION_H
#define MEMBER_FUNCTION_H

#include <string>
#include <vector>
#include <tuple>
#include <any>

#include "TypeDescriptor.hpp"

namespace Reflect
{

	class Function
	{
	public:
		std::string GetName() const { return mName; }
		const TypeDescriptor *GetParent() const { return mParent; }

		template <typename... Args>
		std::any Invoke(std::any& object, Args&&... args) const
		{
			if (sizeof...(Args) == mParamTypes.size())
			{
				std::vector<std::any> anyArgs{ std::any(std::forward<Args>(args))... };

				return InvokeImpl(object, anyArgs);
			}

			return std::any();
		}

		const TypeDescriptor *GetReturnType() const
		{
			return mReturnType;
		}

		std::vector<const TypeDescriptor*> GetParamTypes() const
		{
			return mParamTypes;
		}

		const TypeDescriptor *GetParamType(size_t index) const
		{
			return mParamTypes[index];
		}

		std::size_t GetNumParams() const
		{
			return mParamTypes.size();
		}

	protected:
		Function(const std::string &name, const TypeDescriptor *parent, const TypeDescriptor *returnType, const std::vector<TypeDescriptor const*> paramTypes)
			: mName(name), mParent(parent), mReturnType(returnType), mParamTypes(paramTypes) {}

		const TypeDescriptor *mReturnType;
		std::vector<TypeDescriptor const *> mParamTypes;

	private:
		virtual std::any InvokeImpl(std::any object, std::vector<std::any> &args) const = 0;

		std::string mName;
		TypeDescriptor const *const mParent;
	};


	template <typename Ret, typename... Args>
	class FreeFunction : public Function
	{
	private:
		using FunPtr = Ret(*)(Args...);

	public:
		FreeFunction(FunPtr freeFunPtr, const std::string &name)
			: Function(name, nullptr, Details::Resolve<Ret>(), { Details::Resolve<std::remove_cv_t<std::remove_reference_t<Args>>>()... }), mFreeFunPtr(freeFunPtr) {}

	private:
		std::any InvokeImpl(std::any, std::vector<std::any> &args) const override
		{
			return InvokeImpl(args, std::index_sequence_for<Args...>());
		}

		template <size_t... indices>
		std::any InvokeImpl(std::vector<std::any> &args, std::index_sequence<indices...> indexSequence) const
		{
			std::tuple argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&args[indices])...);
			std::vector<std::any> convertedArgs{ std::get<indices>(argsTuple)... };
			argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&convertedArgs[indices])...);

			if ((std::get<indices>(argsTuple) && ...))  // all arguments are valid
				if constexpr (std::is_reference_v<Ret>)
					return std::any(mFreeFunPtr(*std::get<indices>(argsTuple)...));
				else
					return mFreeFunPtr(*std::get<indices>(argsTuple)...);
			else
				return std::any();
		}

		FunPtr mFreeFunPtr;
	};


	template <typename... Args>
	class FreeFunction<void, Args...> : public Function
	{
	private:
		using FunPtr = void(*)(Args...);

	public:
		FreeFunction(FunPtr freeFunPtr, const std::string &name)
			: Function(name, nullptr, Details::Resolve<void>(), { Details::Resolve<std::remove_cv_t<std::remove_reference_t<Args>>>()... }),
			mFreeFunPtr(freeFunPtr) {}

	private:
		std::any InvokeImpl(std::any, std::vector<std::any> &args) const override
		{
			return InvokeImpl(args, std::index_sequence_for<Args...>());
		}

		template <size_t... indices>
		std::any InvokeImpl(std::vector<std::any> &args, std::index_sequence<indices...> indexSequence) const
		{
			std::tuple argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&args[indices])...);
			std::vector<std::any> convertedArgs{ std::get<indices>(argsTuple)... };
			argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&convertedArgs[indices])...);

			if ((std::get<indices>(argsTuple) && ...))  // all arguments are valid
				mFreeFunPtr(*std::get<indices>(argsTuple)...);

			return std::any();
		}

		FunPtr mFreeFunPtr;
	};


	template <typename C, typename Ret, typename... Args>
	class MemberFunction : public Function
	{
	private:
		using MemFunPtr = Ret(C::*)(Args...);

	public:
		MemberFunction(MemFunPtr memFun, const std::string &name)
			: Function(name, Details::Resolve<C>(), Details::Resolve<Ret>(), { Details::Resolve<std::remove_cv_t<std::remove_reference_t<Args>>>()... }), mMemFunPtr(memFun) {}

	private:
		std::any InvokeImpl(std::any object, std::vector<std::any> &args) const override
		{
			return InvokeImpl(object, args, std::make_index_sequence<sizeof...(Args)>());
		}

		template <size_t... indices>
		std::any InvokeImpl(std::any& object, std::vector<std::any> &args, std::index_sequence<indices...> indexSequence) const
		{
			std::tuple argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&args[indices])...);
			std::vector<std::any> convertedArgs{ std::get<indices>(argsTuple)... };
			argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&convertedArgs[indices])...);

			if (C *obj = std::any_cast<C>(&object); (std::get<indices>(argsTuple) && ...) && obj)  // object is valid and all arguments are valid 
				return (obj->*mMemFunPtr)(*std::get<indices>(argsTuple)...);
			else
				return std::any();
		}

		MemFunPtr mMemFunPtr;
	};


	template <typename C, typename... Args>
	class MemberFunction<C, void, Args...> : public Function
	{
	private:
		using MemFunPtr = void(C::*)(Args...);

	public:
		MemberFunction(MemFunPtr memFun, const std::string &name)
			: Function(name, Details::Resolve<C>(), Details::Resolve<void>(), { Details::Resolve<std::remove_cv_t<std::remove_reference_t<Args>>>()... }),
			mMemFunPtr(memFun) {}

	private:
		std::any InvokeImpl(std::any& object, std::vector<std::any> &args) const override
		{
			return InvokeImpl(object, args, std::make_index_sequence<sizeof...(Args)>());
		}

		template <size_t... indices>
		std::any InvokeImpl(std::any& object, std::vector<std::any> &args, std::index_sequence<indices...> indexSequence) const
		{
			std::tuple argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&args[indices])...);
			std::vector<std::any> convertedArgs{ std::get<indices>(argsTuple)... };
			argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&convertedArgs[indices])...);

			if (C *obj = std::any_cast<C>(&object); (std::get<indices>(argsTuple) && ...) && obj)  // object is valid and all arguments are valid 
				(obj->*mMemFunPtr)(*std::get<indices>(argsTuple)...);

			return std::any();
		}

		MemFunPtr mMemFunPtr;
	};


	template <typename C, typename Ret, typename... Args>
	class ConstMemberFunction : public Function
	{
	private:
		using ConstMemFunPtr = Ret(C::*)(Args...) const;

	public:
		ConstMemberFunction(ConstMemFunPtr constMemFun, const std::string &name)
			: Function(name, Details::Resolve<C>(), Details::Resolve<Ret>(), { Details::Resolve<std::remove_cv_t<std::remove_reference_t<Args>>>()... }), mConstMemFunPtr(constMemFun) {}

	private:
		std::any InvokeImpl(std::any object, std::vector<std::any> &args) const override
		{
			return InvokeImpl(object, args, std::make_index_sequence<sizeof...(Args)>());
		}

		template <size_t... indices>
		std::any InvokeImpl(std::any& object, std::vector<std::any> &args, std::index_sequence<indices...> indexSequence) const
		{
			std::tuple argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&args[indices])...);
			std::vector<std::any> convertedArgs{ std::get<indices>(argsTuple)... };
			argsTuple = std::make_tuple(std::any_cast<std::remove_cv_t<std::remove_reference_t<Args>>>(&convertedArgs[indices])...);

			if (C *obj = std::any_cast<C>(&object); obj && (std::get<indices>(argsTuple) && ...))  // object is valid and all arguments are valid 
				if constexpr (std::is_void<Ret>::value)
				{
					(obj->*mConstMemFunPtr)(*std::get<indices>(argsTuple)...);

					return std::any();
				}
				else
					return (obj->*mConstMemFunPtr)(*std::get<indices>(argsTuple)...);
			else
				return std::any();
		}

		ConstMemFunPtr mConstMemFunPtr;
	};

}  // namespace Reflect

#endif  // MEMBER_FUNCTION_H