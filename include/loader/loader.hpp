/*
** CrabeLoader
** File description:
** loader
*/

#ifndef LOADER_HPP_
#define LOADER_HPP_

class Loader {
    public:
        static Loader& get();

        bool initialize();
        void uninitialize();

    protected:
    private:
        Loader() = default;
        ~Loader() = default;
        Loader(const Loader&) = delete;
        Loader& operator=(const Loader&) = delete;
};


#endif /* !LOADER_HPP_ */
