CFLAGS = -std=c++17 -ggdb 
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

SOURCES := $(shell find src -name '*.cpp')
HEADERS := $(shell find src -name '*.hpp')

VulkanTest: $(SOURCES) $(HEADERS)
	clang++ $(CFLAGS) -o VulkanTest $(SOURCES) $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	bash src/shaders/compile.sh
	./VulkanTest

clean: 
	rm -f VulkanTest
