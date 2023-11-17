int decodeFrame(int decode_pipe[2]);
int encodeFrame(int encode_pipe[2], int crc_flag);
int frameChunk(int frame_pipe[2]);
int deframeFrame(int deframe_pipe[2]);
int malformFrame(int malform_pipe[2], int malform_padding, int crc_flag, int last);
int toUpperFrame(int uppercase_pipe[2]);