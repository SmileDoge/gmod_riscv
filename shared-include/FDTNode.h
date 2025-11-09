#pragma once

#include <string>
#include <memory>

class FDTNode {
public:
	virtual ~FDTNode() = default;
		
	virtual std::unique_ptr<FDTNode> Find(const std::string& name) = 0;
	virtual std::unique_ptr<FDTNode> FindReg(const std::string& name, uint64_t addr) = 0;
	virtual std::unique_ptr<FDTNode> FindRegAny(const std::string& name) = 0;

	virtual std::unique_ptr<FDTNode> CreateChild(const std::string& name) = 0;
	virtual std::unique_ptr<FDTNode> CreateChildReg(const std::string& name, uint64_t addr) = 0;

	virtual void AddProp(const std::string& name, const std::string& val) = 0;
	virtual void AddProp(const std::string& name, uint32_t v) = 0;
	virtual void AddProp(const std::string& name, uint64_t v) = 0;

	virtual void AddProp(const std::string& name, uint64_t addr, uint64_t size) = 0;

	virtual void* GetPropData(const std::string& name) = 0;
	virtual size_t GetPropSize(const std::string& name) = 0;
	virtual bool DeleteProp(const std::string& name) = 0;
};