#ifndef ___MD2_H___
#define ___MD2_H___

// Modified by Dominik Reichl, 2003

class CMD2
{
public:
	CMD2();
	virtual ~CMD2();

	void Init();
	void Update(const unsigned char *buf, unsigned int len);
	void TruncatedFinal(unsigned char *hash, unsigned int size);

private:
	void Transform();
	unsigned char m_X[48], m_C[16], m_buf[16];
	unsigned int m_count;
};

#endif
