#ifndef Animation_h
#define Animation_h

class Animation {
    public:
        //Animation() {};
        
        virtual void Setup() {}
        virtual void Show() = 0;

        virtual int GetName() { return this->Name; }

    protected:
        const static int Name = 1;

};

#endif
