
CFLAGS = -std=c++17 -ggdb 
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

VulkanTest: *.cpp *.hpp
	clang++ $(CFLAGS) -o VulkanTest *.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanTest
	bash shaders/compile.sh
	./VulkanTest

clean: 
	rm -f VulkanTest
