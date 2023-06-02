make clean
cp -r ./* ../../../../../../drivers/k_driver/source/kdrv_builtin/kdrv_ai/
cp ../../include/* ../../../../../../drivers/k_driver/source/kdrv_builtin/kdrv_ai/include/
cp ../../include/kdrv_ai.h  ../../../../../../drivers/k_driver/include/kdrv_ai/
cp ../../include/kdrv_ai_builtin.h  ../../../../../../drivers/k_driver/include/kdrv_ai/
rm -f ../../../../../../drivers/k_driver/source/kdrv_builtin/kdrv_ai/include/kdrv_ai.h
rm -f ../../../../../../drivers/k_driver/source/kdrv_builtin/kdrv_ai/include/kdrv_ai_builtin.h
sed -i 's/#define KDRV_AI_MINI_FOR_FASTBOOT    0/#define KDRV_AI_MINI_FOR_FASTBOOT    1/g' ../../../../../../drivers/k_driver/include/kdrv_ai/kdrv_ai_builtin.h
rm -f ../../../../../../drivers/k_driver/source/kdrv_builtin/kdrv_ai/publish.json
rm -f ../../../../../../drivers/k_driver/source/kdrv_builtin/kdrv_ai/Makefile
sed -i '/MODULE_VERSION(KDRV_AI_IMPL_VERSION);/d' ../../../../../../drivers/k_driver/source/kdrv_builtin/kdrv_ai/ai_main.c
