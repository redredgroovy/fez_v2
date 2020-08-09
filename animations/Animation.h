#ifndef Animation_h
#define Animation_h

class Animation {
    public:

        Animation(const char* name)
        {
            this->name = name;
        };
        
        virtual void Setup() = 0;
        virtual void Loop() = 0;

        virtual const char* Name() { return this->name; }

    protected:
        const char* name;

};

#endif
