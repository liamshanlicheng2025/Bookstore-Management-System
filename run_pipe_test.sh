#!/bin/bash

# 设置路径
EXECUTABLE="./bookstore"
TESTCASE_DIR="testcases/AdvancedTestCase-1"
DATA_DIR="test_data"
OUTPUT_DIR="test_output"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}=== 开始管道符测试 ===${NC}"
echo "测试目录: $TESTCASE_DIR"
echo "数据目录: $DATA_DIR"

# 清理旧数据
echo -e "\n${YELLOW}=== 清理旧数据 ===${NC}"
rm -rf $DATA_DIR $OUTPUT_DIR
mkdir -p $DATA_DIR $OUTPUT_DIR

# 编译程序（如果需要）
echo -e "\n${YELLOW}=== 编译程序 ===${NC}"
make clean && make

if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}错误: 找不到可执行文件 $EXECUTABLE${NC}"
    echo "请先编译程序: make"
    exit 1
fi

# 运行测试点110
echo -e "\n${YELLOW}=== 运行测试点 110 ===${NC}"
echo "输入文件: $TESTCASE_DIR/110.in"

cd $DATA_DIR
time ../$EXECUTABLE < ../$TESTCASE_DIR/110.in > ../$OUTPUT_DIR/output_110.txt 2> ../$OUTPUT_DIR/error_110.txt
cd ..

echo -e "${GREEN}110 标准输出已保存到: $OUTPUT_DIR/output_110.txt${NC}"
echo -e "${GREEN}110 错误输出已保存到: $OUTPUT_DIR/error_110.txt${NC}"

# 显示110的错误输出（如果有）
if [ -s "$OUTPUT_DIR/error_110.txt" ]; then
    echo -e "\n${YELLOW}110 错误输出内容:${NC}"
    cat $OUTPUT_DIR/error_110.txt
fi

# 检查数据库文件
echo -e "\n${YELLOW}=== 110运行后数据库状态 ===${NC}"
if [ -d "$DATA_DIR" ]; then
    echo "数据库文件:"
    ls -la $DATA_DIR/

    # 显示关键文件大小
    for file in $DATA_DIR/*.db; do
        if [ -f "$file" ]; then
            echo "  $(basename $file): $(wc -c < "$file") 字节"
        fi
    done
fi

# 运行测试点111
echo -e "\n${YELLOW}=== 运行测试点 111 ===${NC}"
echo "输入文件: $TESTCASE_DIR/111.in"

cd $DATA_DIR
time ../$EXECUTABLE < ../$TESTCASE_DIR/111.in > ../$OUTPUT_DIR/output_111.txt 2> ../$OUTPUT_DIR/error_111.txt
cd ..

echo -e "${GREEN}111 标准输出已保存到: $OUTPUT_DIR/output_111.txt${NC}"
echo -e "${GREEN}111 错误输出已保存到: $OUTPUT_DIR/error_111.txt${NC}"

# 显示111的错误输出（如果有）
if [ -s "$OUTPUT_DIR/error_111.txt" ]; then
    echo -e "\n${YELLOW}111 错误输出内容:${NC}"
    cat $OUTPUT_DIR/error_111.txt
fi

# 比较输出与标准答案
echo -e "\n${YELLOW}=== 比较输出与标准答案 ===${NC}"

# 比较110的输出
echo "比较 110 输出:"
if [ -f "$TESTCASE_DIR/110.out" ]; then
    diff -u $TESTCASE_DIR/110.out $OUTPUT_DIR/output_110.txt
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}110 输出匹配 ✓${NC}"
    else
        echo -e "${RED}110 输出不匹配 ✗${NC}"
    fi
else
    echo "没有找到 110.out 标准答案文件"
fi

# 比较111的输出
echo -e "\n比较 111 输出:"
if [ -f "$TESTCASE_DIR/111.out" ]; then
    diff -u $TESTCASE_DIR/111.out $OUTPUT_DIR/output_111.txt
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}111 输出匹配 ✓${NC}"
    else
        echo -e "${RED}111 输出不匹配 ✗${NC}"
    fi
else
    echo "没有找到 111.out 标准答案文件"
fi

# 显示关键行对比（方便调试）
echo -e "\n${YELLOW}=== 关键行对比 ===${NC}"
echo "110 输出的最后10行:"
tail -10 $OUTPUT_DIR/output_110.txt
echo -e "\n111 输出的前10行:"
head -10 $OUTPUT_DIR/output_111.txt

# 统计Invalid数量
echo -e "\n${YELLOW}=== Invalid 统计 ===${NC}"
echo "110 中 Invalid 出现次数: $(grep -c "Invalid" $OUTPUT_DIR/output_110.txt)"
echo "111 中 Invalid 出现次数: $(grep -c "Invalid" $OUTPUT_DIR/output_111.txt)"

# 检查是否有buy操作
echo -e "\n${YELLOW}=== Buy 操作统计 ===${NC}"
echo "110 中 buy 命令数量: $(grep -c "buy" $TESTCASE_DIR/110.in)"
echo "110 输出中数值行（可能是buy成功）数量: $(grep -E "^[0-9]+(\.[0-9]+)?" $OUTPUT_DIR/output_110.txt | wc -l)"

echo "111 中 buy 命令数量: $(grep -c "buy" $TESTCASE_DIR/111.in)"
echo "111 输出中数值行（可能是buy成功）数量: $(grep -E "^[0-9]+(\.[0-9]+)?" $OUTPUT_DIR/output_111.txt | wc -l)"

echo -e "\n${YELLOW}=== 测试完成 ===${NC}"
echo "输出文件保存在: $OUTPUT_DIR/"
echo "数据文件保存在: $DATA_DIR/"