using System.Security.Cryptography;

using System.Xml.Serialization;

using System.Text;

namespace Cryptography
{
    class RSA
    {
        public RSACryptoServiceProvider? RSACSP = null;
        public RSAParameters? RSAPrivateKey = null;
        public RSAParameters? RSAPublicKey = null;

        public Encoding DefaultEncoding = Encoding.Unicode;

        public RSA(int keySize = 4096)
        {
            try
            {
                RSACSP = new(keySize);

                RSAPrivateKey = RSACSP.ExportParameters(true);
                RSAPublicKey = RSACSP.ExportParameters(false);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        public byte[]? Encrypt(byte[] data, bool doOAEPPadding = true)
        {
            if (RSACSP == null || RSAPublicKey == null)
            {
                return null;
            }

            try
            {
                RSACSP.ImportParameters((RSAParameters)RSAPublicKey);
                return RSACSP.Encrypt(data, doOAEPPadding);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return null;
            }
        }

        public byte[]? EncryptAsString(string data, bool doOAEPPadding = true)
        {
            try
            {
                return Encrypt(DefaultEncoding.GetBytes(data), doOAEPPadding);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return null;
            }
        }

        public byte[]? Decrypt(byte[] data, bool doOAEPPadding = true)
        {
            if (RSACSP == null || RSAPrivateKey == null)
            {
                return null;
            }

            try
            {
                RSACSP.ImportParameters((RSAParameters)RSAPrivateKey);
                return RSACSP.Decrypt(data, doOAEPPadding);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return null;
            }
        }

        public string DecryptAsString(byte[] data, bool doOAEPPadding = true)
        {
            var dataAsBytes = Decrypt(data, doOAEPPadding);
            if (dataAsBytes == null)
            {
                return "";
            }

            try
            {
                return DefaultEncoding.GetString(dataAsBytes);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return "";
            }
        }

        private string GetKey(bool publicKey = true)
        {
            try
            {
                StringWriter stringWriter = new();
                XmlSerializer xmlSerializer = new(typeof(RSAParameters));

                xmlSerializer.Serialize(stringWriter, (publicKey ? RSAPublicKey : RSAPrivateKey));

                return stringWriter.ToString();
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return "";
            }
        }

        public string GetKeyPublicAsString()
        {
            return GetKey();
        }

        public string GetKeyPrivateAsString()
        {
            return GetKey(false);
        }

        public static RSAParameters? StringToKeys(string str)
        {
            try
            {
                StringReader stringReader = new(str);
                XmlSerializer xmlSerializer = new(typeof(RSAParameters));

                return (RSAParameters?)xmlSerializer.Deserialize(stringReader);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return null;
            }
        }
    }
}
