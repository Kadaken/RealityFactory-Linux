int RF_TestHumanHost(const char *profile);
int main(int argc, char **argv)
{
    return argc == 2 ? RF_TestHumanHost(argv[1]) : 2;
}
