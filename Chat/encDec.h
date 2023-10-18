int decodeFrame(int decode_pipe[2]);
int encodeFrame(int encode_pipe[2]);
int frameChunk(int frame_pipe[2]);
int deframeFrame(int deframe_pipe[2]);
int malformFrame(int malform_pipe[2]);
int toUpperFrame(int uppercase_pipe[2]);