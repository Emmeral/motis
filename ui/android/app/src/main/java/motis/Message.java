// automatically generated by the FlatBuffers compiler, do not modify

package motis;

import java.nio.*;
import java.lang.*;
import java.util.*;
import com.google.flatbuffers.*;

@SuppressWarnings("unused")
public final class Message extends Table {
  public static Message getRootAsMessage(ByteBuffer _bb) { return getRootAsMessage(_bb, new Message()); }
  public static Message getRootAsMessage(ByteBuffer _bb, Message obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__init(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public Message __init(int _i, ByteBuffer _bb) { bb_pos = _i; bb = _bb; return this; }

  public Destination destination() { return destination(new Destination()); }
  public Destination destination(Destination obj) { int o = __offset(4); return o != 0 ? obj.__init(__indirect(o + bb_pos), bb) : null; }
  public byte contentType() { int o = __offset(6); return o != 0 ? bb.get(o + bb_pos) : 0; }
  public Table content(Table obj) { int o = __offset(8); return o != 0 ? __union(obj, o) : null; }
  public int id() { int o = __offset(10); return o != 0 ? bb.getInt(o + bb_pos) : 0; }

  public static int createMessage(FlatBufferBuilder builder,
      int destinationOffset,
      byte content_type,
      int contentOffset,
      int id) {
    builder.startObject(4);
    Message.addId(builder, id);
    Message.addContent(builder, contentOffset);
    Message.addDestination(builder, destinationOffset);
    Message.addContentType(builder, content_type);
    return Message.endMessage(builder);
  }

  public static void startMessage(FlatBufferBuilder builder) { builder.startObject(4); }
  public static void addDestination(FlatBufferBuilder builder, int destinationOffset) { builder.addOffset(0, destinationOffset, 0); }
  public static void addContentType(FlatBufferBuilder builder, byte contentType) { builder.addByte(1, contentType, 0); }
  public static void addContent(FlatBufferBuilder builder, int contentOffset) { builder.addOffset(2, contentOffset, 0); }
  public static void addId(FlatBufferBuilder builder, int id) { builder.addInt(3, id, 0); }
  public static int endMessage(FlatBufferBuilder builder) {
    int o = builder.endObject();
    return o;
  }
  public static void finishMessageBuffer(FlatBufferBuilder builder, int offset) { builder.finish(offset); }
};

