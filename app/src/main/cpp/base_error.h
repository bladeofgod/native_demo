//
// Created by Admin on 2021/6/9.
//

#include <cstring>

#ifndef NATIVEDEMO_BASE_ERROR_H
#define NATIVEDEMO_BASE_ERROR_H


class BaseError{
private:
    int err_no;
    char error_msg[255];
public:
    BaseError(int err_no = -1,const char *err_msg = "未知错误") {
        this->err_no = err_no;
        strcpy(error_msg,err_msg);
    }

    int getErrorNo() {
        return err_no;
    }

    const char* getErrorMsg() const {
        return error_msg;
    }


};


#endif //NATIVEDEMO_BASE_ERROR_H
