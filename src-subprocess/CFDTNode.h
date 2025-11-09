#pragma once

#include "FDTNode.h"

extern "C"
{
#include "fdtlib.h"
}

class FDTNodeImpl : public FDTNode
{
public:
	FDTNodeImpl(fdt_node* node) : node_(node) {}
	~FDTNodeImpl() override = default;

	std::unique_ptr<FDTNode> Find(const std::string& name) override {
		fdt_node* child = fdt_node_find(node_, name.c_str());
		if (!child) return nullptr;
		return std::make_unique<FDTNodeImpl>(child);
	}

	std::unique_ptr<FDTNode> FindReg(const std::string& name, uint64_t addr) override {
		fdt_node* child = fdt_node_find_reg(node_, name.c_str(), addr);
		if (!child) return nullptr;
		return std::make_unique<FDTNodeImpl>(child);
	}

	std::unique_ptr<FDTNode> FindRegAny(const std::string& name) override {
		fdt_node* child = fdt_node_find_reg_any(node_, name.c_str());
		if (!child) return nullptr;
		return std::make_unique<FDTNodeImpl>(child);
	}

	std::unique_ptr<FDTNode> CreateChild(const std::string& name) override {
		fdt_node* child = fdt_node_create(name.c_str());
		fdt_node_add_child(node_, child);
		return std::make_unique<FDTNodeImpl>(child);
	}

	std::unique_ptr<FDTNode> CreateChildReg(const std::string& name, uint64_t addr) override {
		fdt_node* child = fdt_node_create_reg(name.c_str(), addr);
		fdt_node_add_child(node_, child);
		return std::make_unique<FDTNodeImpl>(child);
	}


	void AddProp(const std::string& name, const std::string& val) override {
		fdt_node_add_prop_str(node_, name.c_str(), val.c_str());
	}

	void AddProp(const std::string& name, uint32_t v) override {
		fdt_node_add_prop_u32(node_, name.c_str(), v);
	}

	void AddProp(const std::string& name, uint64_t v) override {
		fdt_node_add_prop_u64(node_, name.c_str(), v);
	}

	void AddProp(const std::string& name, uint64_t addr, uint64_t size) override {
		fdt_node_add_prop_reg(node_, name.c_str(), addr, size);
	}


	void* GetPropData(const std::string& name) override {
		return fdt_node_get_prop_data(node_, name.c_str());
	}

	size_t GetPropSize(const std::string& name) override {
		return fdt_node_get_prop_size(node_, name.c_str());
	}


	bool DeleteProp(const std::string& name) override {
		return fdt_node_del_prop(node_, name.c_str());
	}
private:
	fdt_node* node_;
};